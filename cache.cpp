#include "cache.hpp"

#include <iostream>

#if DEBUG
#include <cstdio>
#endif

#define ADDR_SIZE 64

namespace {
    //helper methods
    inline uint64_t get_mask(uint64_t n) {
        return (1 << n) - 1;       
    }
}

Cache::Cache(const cache_config_t& config, bool isL1) : 
    m_Config(config),
    m_nSets(1 << (config.c - config.b - config.s)),
    m_Associativity(1 << config.s),
    m_TimestepCounter(1 << (config.c - config.b + 1)),
    m_IsL1(isL1),
    m_PreviousMissLoc(0x0)
{
    const uint64_t numBlocks = m_nSets*m_Associativity; //Alternatively 2^(C - B)
    m_Entries = new cache_entry_t[numBlocks];
    for (uint64_t i = 0; i < numBlocks; i++) {
        m_Entries[i].valid = false;
        m_Entries[i].dirty = false;
    }
}

/**
 * Returns true on hit, false on miss.
 * Returns false automatically if disabled.
 * 
 * In the event of an L2 write, we still need to check the cache due to replacement policies,
 * however the return value of the L2 cache is unused since it is the LLC.
 * 
 * Uses "needsWriteback" and "writeback" to signify a writeback in the case of WBWA.
*/
bool Cache::access(char rw, uint64_t tag, uint64_t index, sim_stats_t* stats) {
    if (m_IsL1) {
        stats->accesses_l1++;
    } else {
        stats->accesses_l2++;
        if (rw == 'R') {
            stats->reads_l2++;
        } else {
            stats->writes_l2++;
        }
    }
    
    if (m_Config.disabled) {
        if (rw == 'R') {
            stats->read_misses_l2++;
            #if DEBUG
            printf("L2 is disabled, treating this as an L2 read miss\n");
            #endif
        } else {
            #if DEBUG
                printf("L2 is disabled, writing through to memory\n");
            #endif
        }
        return false;
    }
    
    //Check Hit
    cache_entry_t* hit = block_in_cache(tag, index);
    if (hit != nullptr) {
        //CACHE HIT
        #if DEBUG
        if (m_IsL1) {
            printf("L1 hit\n");
        } else {
            if (rw == 'R') {
                printf("L2 read hit\n");
            } else {
                printf("L2 found block in cache on write\n");
            }
        }
        #endif

        //Handle Replacement Updates
        if (m_Config.replace_policy == REPLACE_POLICY_LFU) {
            hit->useCounter++;
            clear_mru(index);
            hit->mru = true;
        } else {
            hit->timestamp = m_TimestepCounter;
            m_TimestepCounter++;
        }
        #if DEBUG
        if (m_IsL1) {
            printf("In L1, moving Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 " to MRU position", tag, index);
        } else {
            printf("In L2, moving Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 " to MRU position", tag, index);
        }
        #endif

        //Handle Writeback
        if (rw == 'W' && m_Config.write_strat == WRITE_STRAT_WBWA) {
            #if DEBUG
            printf(" and setting dirty bit\n");
            #endif
            hit->dirty = true;
        }
        #if DEBUG
        else {
            printf("\n");
        }
        #endif

        if (m_IsL1) {
            stats->hits_l1++;
        } else if (rw == 'R') {
            stats->read_hits_l2++;
        }
    } else {
        //CACHE MISS
        if (m_IsL1) {
            #if DEBUG
            printf("L1 miss\n");
            #endif
            stats->misses_l1++;
        } else if (rw == 'R') {
            #if DEBUG
            printf("L2 read miss\n");
            #endif
            stats->read_misses_l2++;
        }
        #if DEBUG
        else {
            printf("L2 did not find block in cache on write, writing through to memory anyway\n");
        }
        #endif
    }

    return hit;
}

/**
 * Does insertion of block.
*/
void Cache::install(char rw, uint64_t tag, uint64_t index, sim_stats_t* stats, bool* outNeedsWB, uint64_t* outWBAddr) {
    cache_entry_t& installBlock = find_eviction_block(index);

    #if DEBUG
        printf("Evict from %s: ", m_IsL1 ? "L1" : "L2");
    #endif

    bool needsWB = false;
    uint64_t wbAddr;
    if (installBlock.valid) {
        //Evict block
        stats->num_evictions++;
        if (installBlock.dirty && m_Config.write_strat == WRITE_STRAT_WBWA) {
            needsWB = true;
            wbAddr = get_addr(installBlock.tag, index);
        }
        #if DEBUG
        if (m_IsL1) {
            printf("block with valid=1, dirty=%d, tag 0x%" PRIx64 " and index=0x%" PRIx64 "\n", 
                installBlock.dirty, installBlock.tag, index);
        } else {
            printf("block with valid=1, tag 0x%" PRIx64 " and index=0x%" PRIx64 "\n", 
                installBlock.tag, index);
        }
        #endif
    }
    #if DEBUG
    else {
        printf("block with valid=0 and index=0x%" PRIx64 "\n", index);
    }
    #endif

    installBlock.valid = true;
    installBlock.tag = tag;
    const bool writeAllocate = rw == 'W' && m_Config.write_strat == WRITE_STRAT_WBWA;
    installBlock.dirty = writeAllocate;

    //Handle MIP for LRU and LFU
    if (m_Config.replace_policy == REPLACE_POLICY_LFU) {
        installBlock.useCounter = 1;
        clear_mru(index);
        installBlock.mru = true;
    } else {
        installBlock.timestamp = m_TimestepCounter;
        m_TimestepCounter++;
    }

    if (outNeedsWB != nullptr) {
        *outNeedsWB = needsWB;
    }
    if (outWBAddr != nullptr && needsWB) {
        *outWBAddr = wbAddr;
    }
}

bool Cache::find_prefetch_target(uint64_t tag, uint64_t index, uint64_t* prefetch_tag, uint64_t* prefetch_index) {
    const uint64_t indexBits = m_Config.c - m_Config.b - m_Config.s;
    uint64_t tagIndex = (tag << indexBits) | index;
    if (m_Config.prefetcher_disabled) {
        return false;
    } 
    
    uint64_t prefetch_loc;
    if (m_Config.strided_prefetch_disabled) {
        prefetch_loc = tagIndex + 1;
    } else {
        //TODO: Implement Strided Prefetch
        prefetch_loc = tagIndex + (tagIndex - m_PreviousMissLoc);
        m_PreviousMissLoc = tagIndex;
    }
    *prefetch_index = prefetch_loc & get_mask(indexBits);
    prefetch_loc >>= indexBits;
    *prefetch_tag = prefetch_loc;
    return true;
}

void Cache::prefetch_install(uint64_t tag, uint64_t index, sim_stats_t* stats) {
    if (block_in_cache(tag, index) != nullptr) {
        //NO PREFETCH
        return;
    }

    #if DEBUG
    uint64_t indexBits = m_Config.c - m_Config.b - m_Config.s;
    uint64_t debugAddr = (((tag << indexBits) | index)<< m_Config.b);
    printf("Prefetch block with address 0x%" PRIx64 " from memrory to L2\n", debugAddr);
    #endif

    uint64_t lowestTimestamp;
    bool lowestDefined;
    cache_entry_t& installBlock = find_eviction_block(index, &lowestDefined, &lowestTimestamp);

    stats->prefetches_l2++;
    installBlock.valid = true;
    installBlock.tag = tag;
    //Handle Replacement Policy Updates
    if (m_Config.replace_policy == REPLACE_POLICY_LFU) {
        if (m_Config.prefetch_insert_policy == INSERT_POLICY_LIP) {
            installBlock.mru = false;
        } else {
            clear_mru(index);
            installBlock.mru = true;
        }

        installBlock.useCounter = 0;
    } else {
        if (m_Config.prefetch_insert_policy == INSERT_POLICY_LIP && lowestDefined) {
            //LIP
            installBlock.timestamp = lowestTimestamp-1;
        } else {
            //MIP
            installBlock.timestamp = m_TimestepCounter;
        }

        m_TimestepCounter++;
    }
 
}

void Cache::parse_addr(uint64_t addr, uint64_t* tag, uint64_t* index/*, uint64_t* offset*/) {
    uint64_t tempAddr = addr;
    //*offset = addr & get_mask(m_Config.b);  //Block offset doesn't actually matter for simulation.
    tempAddr >>= m_Config.b;
    uint64_t indexBits = m_Config.c - m_Config.b - m_Config.s;
    *index = tempAddr & get_mask(indexBits);
    tempAddr >>= indexBits;

    //Right shift has some weird artifacts sometimes due to 1 filling, so we mask the tag anyway
    //uint64_t tagBits = ADDR_SIZE-(m_Config.c-m_Config.s);   //C-S = bits in index + offset.
    //*tag = addr & get_mask(tagBits);
    *tag = tempAddr;

#if DEBUG
if (m_IsL1) {
    printf("L1 decomposed address 0x%" PRIx64 " -> Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 "\n", addr, *tag, *index);
} else {
    printf("L2 decomposed address 0x%" PRIx64 " -> Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 "\n", addr, *tag, *index);
}
#endif
}

double Cache::get_hit_time() {
    if (m_Config.disabled) {
        return 0.0;
    }

    double k1_term = m_Config.c - m_Config.b - m_Config.s;
    double k2_term = m_Config.s > 3 ? m_Config.s - 3 : 0;
    double k0, k1, k2;
    if (m_IsL1) {
        k0 = K_L1[0];
        k1 = K_L1[1];
        k2 = K_L1[2];
    } else {
        k0 = K_L2[0];
        k1 = K_L2[1];
        k2 = K_L2[2];
    }

    return k0 + k1 * k1_term + k2 * k2_term;
}

bool Cache::disabled() {
    return m_Config.disabled;
}

void Cache::print_contents() {
    const uint64_t numBlocks = m_nSets*m_Associativity;
    for (size_t i = 0; i < numBlocks; i++) {
        printf("Block %ld: Tag: %" PRIu64 "\n", i, m_Entries[i].tag);       
    }
}

cache_entry_t& Cache::find_eviction_block(uint64_t index, bool* outLowestDefined, uint64_t* outLowestTimestamp) {
    bool foundEmptyBlock = false;
    bool lowestDefined = false;
    uint64_t lowestTimestamp;
    uint64_t lowestCounter;
    cache_entry_t* evictBlock;
    for (uint32_t b = index*m_Associativity; b < (index+1)*m_Associativity; b++) {
        cache_entry_t& block = m_Entries[b];
        if (!block.valid) {
            //Install in empty block
            evictBlock = &block;
            foundEmptyBlock = true;
        } else {
            bool foundNewEvictBlock = false;
            if (m_Config.replace_policy == REPLACE_POLICY_LFU) {
                //LFU Replacement
                if (!block.mru) {
                    if (!lowestDefined || block.useCounter < lowestCounter) {
                        lowestDefined = true;
                        lowestCounter = block.useCounter;
                        foundNewEvictBlock = true;
                    } else if (block.useCounter == lowestCounter && block.tag < evictBlock->tag) {
                        foundNewEvictBlock = true;
                    }
                }
            } else {
                //LRU Replacement
                if (!lowestDefined || block.timestamp < lowestTimestamp) {
                    //evict block with lowest timestamp
                    lowestDefined = true;
                    lowestTimestamp = block.timestamp;
                    foundNewEvictBlock = true;
                }
            }

            if (!foundEmptyBlock && foundNewEvictBlock) {
                evictBlock = &block;
            }
        }
    }

    if (outLowestDefined != nullptr) {
        *outLowestDefined = lowestDefined;
    }
    if (outLowestTimestamp != nullptr) {
        *outLowestTimestamp = lowestTimestamp;
    }
    return *evictBlock;
}

cache_entry_t* Cache::block_in_cache(uint64_t tag, uint64_t index) {
    for (uint32_t b = index*m_Associativity; b < (index+1)*m_Associativity; b++) {
        cache_entry_t& block = m_Entries[b];
        if (block.valid && block.tag == tag) {
            return &block;
        }
    }

    return nullptr;
}

uint64_t Cache::get_addr(uint64_t tag, uint64_t index) {
    return (tag << (m_Config.c - m_Config.s)) | (index << m_Config.b);
}

void Cache::clear_mru(uint64_t index) {
    for (uint32_t b = index*m_Associativity; b < (index+1)*m_Associativity; b++) {
        cache_entry_t& block = m_Entries[b];
        block.mru = false;
    }
}