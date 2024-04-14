//Compressed Window: 1 cacheline
//Uncompressed Window: 2 cachelines.

//Simulate Last Level Cache
//Assume 8 way set associative cache of 32KB (Actual is 32MB)
//512 cache lines @ 64 bytes per line

#ifndef CACHE_HPP
#define CACHE_HPP

#include <inttypes.h>
#include <stddef.h>

#include "cache_sim.hpp"

typedef struct cache_entry {
    uint64_t tag;
    uint64_t timestamp;
    bool valid;
    bool dirty; //Only used for WB/WA Policy.

    //LFU
    uint64_t useCounter;
    bool mru;
} cache_entry_t;

//l1 and l2 will use same cache struct with different configurations.
class Cache {
private:
    const cache_config_t& m_Config;
    uint64_t m_nSets; 
    uint64_t m_Associativity;
    uint64_t m_TimestepCounter;
    const bool m_IsL1; //This is purely for accessing the right statistics.
    cache_entry_t* m_Entries;  //2D set-major array to represent set associativity.

    //Strided Prefetch
    uint64_t m_PreviousMissLoc;

public:
    Cache(const cache_config_t& config, bool isL1);
    bool access(char rw, uint64_t tag, uint64_t offset, sim_stats_t* stats);
    void install(char rw, uint64_t tag, uint64_t index, sim_stats_t* stats, bool* outNeedsWB=nullptr, uint64_t* outWBAddr=nullptr);
    bool find_prefetch_target(uint64_t tag, uint64_t index, uint64_t* prefetch_tag, uint64_t* prefetch_index);
    void prefetch_install(uint64_t tag, uint64_t index, sim_stats_t* stats);
    void parse_addr(uint64_t addr, uint64_t* tag, uint64_t* index/*, uint64_t* offset*/);
    double get_hit_time();
    bool disabled();

    void print_contents();
private:
    cache_entry_t& find_eviction_block(uint64_t index, bool* outLowestDefined=nullptr, uint64_t* outLowestTimestamp=nullptr);
    cache_entry_t* block_in_cache(uint64_t tag, uint64_t index);
    uint64_t get_addr(uint64_t tag, uint64_t index);
    void clear_mru(uint64_t index);
};

const double K_L1[] = {1, 0.15, 0.15};
const double K_L2[] = {4, 0.3, 0.3};
#endif