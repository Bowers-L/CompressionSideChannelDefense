#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum replace_policy {
    // LRU replacement
    REPLACE_POLICY_LRU,
    // LFU replacement
    REPLACE_POLICY_LFU
} replace_policy_t;

typedef enum insert_policy {
    INSERT_POLICY_MIP,
    INSERT_POLICY_LIP,
} insert_policy_t;

typedef enum write_strat {
    // Write back, write-allocate
    WRITE_STRAT_WBWA,
    // Write through, write-no-allocate
    WRITE_STRAT_WTWNA,
} write_strat_t;

typedef struct cache_config {
    bool disabled;
    bool prefetcher_disabled;
    bool strided_prefetch_disabled;

    // (C,B,S) in the Conte Cache Taxonomy (Patent Pending)
    uint64_t c;
    uint64_t b;
    uint64_t s;
    replace_policy_t replace_policy;
    insert_policy_t prefetch_insert_policy;
    write_strat_t write_strat;
} cache_config_t;

typedef struct sim_config {
    cache_config_t l1_config;
    cache_config_t l2_config;
} sim_config_t;

typedef struct sim_stats {
    uint64_t reads;
    uint64_t writes;
    uint64_t accesses_l1;
    uint64_t reads_l2;
    uint64_t writes_l2;
    uint64_t accesses_l2;
    uint64_t hits_l1;
    uint64_t read_hits_l2;
    uint64_t misses_l1;
    uint64_t read_misses_l2;
    uint64_t prefetches_l2;
    double hit_ratio_l1;
    double read_hit_ratio_l2;
    double miss_ratio_l1;
    double read_miss_ratio_l2;
    double avg_access_time_l1;
    double avg_access_time_l2;

    //Added Statistics
    uint64_t num_evictions;
    double llc_walk_time;
} sim_stats_t;

extern void sim_setup(sim_config_t *config);
extern double sim_access(char rw, uint64_t addr, sim_stats_t* p_stats);
extern void sim_finish(sim_stats_t *p_stats);

extern void print_cache_contents();

// Sorry about the /* comments */. C++11 cannot handle basic C99 syntax,
// unfortunately
static const sim_config_t DEFAULT_SIM_CONFIG = {
    /*.l1_config =*/ {/*.disabled =*/ false,
                      /*.prefetcher_disabled =*/ true,
                      /*.strided_prefetch_disabled =*/ true,
                      /*.c =*/ 10, // 1KB Cache
                      /*.b =*/ 6,  // 64-byte blocks
                      /*.s =*/ 1,  // 2-way
                      /*.replace_policy =*/ REPLACE_POLICY_LRU,
                      /*.prefetch_insert_policy =*/ INSERT_POLICY_MIP,
                      /*.write_strat =*/ WRITE_STRAT_WBWA},

    /*.l2_config =*/ {/*.disabled =*/ false,
                      /*.prefetcher_disabled =*/ false,
                      /*.strided_prefetch_disabled =*/ false,
                      /*.c =*/ 15, // 32KB Cache
                      /*.b =*/ 6,  // 64-byte blocks
                      /*.s =*/ 3,  // 8-way
                      /*.replace_policy =*/ REPLACE_POLICY_LRU,
                      /*.prefetch_insert_policy =*/ INSERT_POLICY_LIP,
                      /*.write_strat =*/ WRITE_STRAT_WTWNA}
};

// Argument to cache_access rw. Indicates a load
static const char READ = 'R';
// Argument to cache_access rw. Indicates a store
static const char WRITE = 'W';

static const double DRAM_ACCESS_TIME = 100;
static const double L1_HIT_K0 = 1;
static const double L1_HIT_K1 = 0.15;
static const double L1_HIT_K2 = 0.15;
static const double L2_HIT_K3 = 4;
static const double L2_HIT_K4 = 0.3;
static const double L2_HIT_K5 = 0.3;

#endif /* CACHESIM_HPP */
