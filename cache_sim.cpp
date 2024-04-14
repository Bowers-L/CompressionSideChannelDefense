//Simulates an L1+L2 Cache
//L2 can be disabled to simulate a single cache.

#include "cache.hpp"

#define HIT_TIME 30.5
#define MISS_TIME 40.7

#if DEBUG
#include <cstdio>
#endif

static Cache* l1;
static Cache* l2;
static uint64_t time;

void sim_setup(sim_config_t *config) {
    l1 = new Cache(config->l1_config, true);
    l2 = new Cache(config->l2_config, false);
    time = 0;
}

//Returns the time required to access
double sim_access(char rw, uint64_t addr, sim_stats_t* stats) {
    #if DEBUG
    printf("Time: %" PRIu64 ". Address: 0x%" PRIx64 ". Read/Write: %c\n", time, addr, rw);
    #endif
    if (rw == 'R') {
        stats->reads++;
    } else {
        stats->writes++;
    }

    uint64_t l1_tag, l1_index;
    l1->parse_addr(addr, &l1_tag, &l1_index);
    bool hit = true;
    if (!l1->access(rw, l1_tag, l1_index, stats)) {

        //Access L2 cache on L1 miss
        //NOTE: Write miss is still a read for L2 due to WB policy. 
        uint64_t l2_tag, l2_index;
        l2->parse_addr(addr, &l2_tag, &l2_index);
        if (!l2->access('R', l2_tag, l2_index, stats) && !l2->disabled()) {
            l2->install('R', l2_tag, l2_index, stats);

            uint64_t prefetch_tag, prefetch_index;
            if (l2->find_prefetch_target(l2_tag, l2_index, &prefetch_tag, &prefetch_index)) {
                l2->prefetch_install(prefetch_tag, prefetch_index, stats);
            }
        }

        bool needsWriteback;
        uint64_t wbAddr; //Note: this address will always have a block offset of 0.
        l1->install(rw, l1_tag, l1_index, stats, &needsWriteback, &wbAddr);
        if (needsWriteback) {
            #if DEBUG
            printf("Writing back dirty block with address 0x%" PRIx64 " to L2\n", wbAddr);
            #endif
            uint64_t l2_wb_tag, l2_wb_index;
            l2->parse_addr(wbAddr, &l2_wb_tag, &l2_wb_index);
            l2->access('W', l2_wb_tag, l2_wb_index, stats);
        }

        hit = false;
    }

    time++;
    #if DEBUG
    printf("\n");
    #endif

    if (hit) {
        return HIT_TIME;
    } else {
        return HIT_TIME + MISS_TIME;
    }
}

void sim_finish(sim_stats_t *stats) {
    //Calculate stats
    stats->hit_ratio_l1 = (double) stats->hits_l1 / stats->accesses_l1;
    stats->miss_ratio_l1 = (double) stats->misses_l1 / stats->accesses_l1;
    // stats->read_hit_ratio_l2 = (double) stats->read_hits_l2 / stats->reads_l2;
    // stats->read_miss_ratio_l2 = (double) stats->read_misses_l2 / stats->reads_l2;

    //stats->avg_access_time_l2 = l2->get_hit_time() + stats->read_miss_ratio_l2*100.0;

    stats->avg_access_time_l1 = HIT_TIME + stats->miss_ratio_l1 * MISS_TIME;

    //Clear Memory
    delete l1;
    delete l2;
}

void print_cache_contents() {
    l1->print_contents();
}