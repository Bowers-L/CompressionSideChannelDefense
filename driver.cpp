//Stdlib Things
#include <iostream>
#include <stdlib.h>
#include "time.h"

//My Things
#include "cache_sim.hpp"
#include "frame.hpp"

#define TIMING_THRESHOLD_BASELINE 17
#define TIMING_THRESHOLD_FACADE 34
#define NUM_LAYERS 10

#define C 16
#define B 6
#define S 10    //C-B for full associativity

// typedef struct {
//     double accuracy;
//     double avg_render_time;
// } stats_t;

typedef struct {
    uint64_t num_evictions;
    double walk_time;
} llc_walk_stats_t;

typedef struct {
    uint64_t texture_size;
    llc_walk_stats_t uncompressed;
    llc_walk_stats_t compressed;
} llc_walk_stats_combined_t;


// static stats_t stats_baseline;
// static stats_t stats_facade;

//Cache
static sim_config_t cache_config;

static std::string channel_to_str[] = {
    "Red", "Green", "Blue", "Alpha"
};

// static std::string bool_to_string(bool value) {
//     return value ? "Yes" : "No";
// }

// static void print_compress_result(std::string name, const compress_result_t* result) {
//     printf("Compressed pattern: %s\n", name.c_str());
//     printf("\tDid Compression: %s\n", bool_to_string(result->did_compression).c_str());
//     printf("\tCompression Ratio: %.2f\n", result->compress_ratio);
//     printf("\tLLC Time: %.2f\n", result->llc_time);
//     for (size_t c = 0; c < 4; c++) {
//         printf("\t%s Channel\n", channel_to_str[c].c_str());
//         printf("\t\tSkip Bit: %d\n", result->skip[c]);
//         printf("\t\t Prediction: %d\n", result->prediction[c]);
//         printf("\t\t NumBits: %d\n", result->numBits[c]);
//     }
//     printf("\n");
// }

// void print_stats(stats_t* stats) {
//     printf("\tAccuracy: %.4f\n", stats->accuracy);
//     printf("\tAverage Render Time: %.4f\n", stats->avg_render_time);
// }

static void print_cache_stats(sim_stats_t* stats) {
    printf("Cache Statistics\n");
    printf("----------------\n");
    printf("Reads: %" PRIu64 "\n", stats->reads);
    printf("Writes: %" PRIu64 "\n", stats->writes);
    printf("\n");
    printf("L1 accesses: %" PRIu64 "\n", stats->accesses_l1);
    printf("L1 hits: %" PRIu64 "\n", stats->hits_l1);
    printf("L1 misses: %" PRIu64 "\n", stats->misses_l1);
    printf("L1 hit ratio: %.3f\n", stats->hit_ratio_l1);
    printf("L1 miss ratio: %.3f\n", stats->miss_ratio_l1);
    printf("L1 average access time (AAT): %.3f\n", stats->avg_access_time_l1);
    printf("\n");
    printf("Number of Evictions: %" PRIu64 "\n", stats->num_evictions);
    printf("LLC Walk Time: %.3f\n", stats->llc_walk_time);
    // printf("\n");
    // printf("L2 reads: %" PRIu64 "\n", stats->reads_l2);
    // printf("L2 writes: %" PRIu64 "\n", stats->writes_l2);
    // printf("L2 read hits: %" PRIu64 "\n", stats->read_hits_l2);
    // printf("L2 read misses: %" PRIu64 "\n", stats->read_misses_l2);
    // printf("L2 prefetches: %" PRIu64 "\n", stats->prefetches_l2);
    // printf("L2 read hit ratio: %.3f\n", stats->read_hit_ratio_l2);
    // printf("L2 read miss ratio: %.3f\n", stats->read_miss_ratio_l2);
    // printf("L2 average access time (AAT): %.3f\n", stats->avg_access_time_l2);
}

// static bool correct_guess(bool guess_white, uint8_t* pixel) {
//     bool correct_guess = (guess_white && pixel[0] > 128) || (!guess_white && pixel[0] < 128);
//     return correct_guess;
// }

static void init_cache_config() {
    cache_config.l1_config.c = C;
    cache_config.l1_config.b = B;
    cache_config.l1_config.s = S;   //S=0: Direct Mapped, S=3: 8-Way Set Associative, S=10 (16-6): Fully Associative
    cache_config.l1_config.replace_policy = REPLACE_POLICY_LRU;
    cache_config.l1_config.write_strat = WRITE_STRAT_WBWA;
    cache_config.l1_config.prefetcher_disabled = true;

    //Don't use second cache.
    cache_config.l2_config.disabled = true;
}

static void init_stats(sim_stats_t* stats) {
    stats->accesses_l1 = 0;
    stats->reads = 0;
    stats->writes = 0;
    stats->hits_l1 = 0;
    stats->misses_l1 = 0;
    stats->num_evictions = 0;
    stats->llc_walk_time = 0;
}

void measure_llc_walk(size_t num_windows, llc_walk_stats_combined_t* stats, bool use_facade = false) {
    sim_stats_t cache_stats_black;
    init_stats(&cache_stats_black);
    sim_stats_t cache_stats_noise;
    init_stats(&cache_stats_noise);

    frame_t* buffer_frame = get_new_frame_random(FRAME_NUM_WINDOWS_CACHE);
    frame_t* frame_black = get_new_frame_black(num_windows);
    frame_t* frame_noise = get_new_frame_random(num_windows);
    //print_frame_nWindows();

    //RUN COMPRESSED LAYERS

    sim_setup(&cache_config);
    read_frame(buffer_frame, &cache_stats_black);    //Dummy frame to fill entire LLC
    if (use_facade) {
        read_frame_facade(frame_black, &cache_stats_black);
    } else {
        read_frame(frame_black, &cache_stats_black);
    }
    cache_stats_black.llc_walk_time = read_frame_backwards(buffer_frame, &cache_stats_black);
    sim_finish(&cache_stats_black);

    // print_cache_stats(&cache_stats_black);

    stats->compressed.num_evictions = cache_stats_black.num_evictions;
    stats->compressed.walk_time = cache_stats_black.llc_walk_time;

    //RUN UNCOMPRESSED LAYERS
    sim_setup(&cache_config);
    read_frame(buffer_frame, &cache_stats_noise);    //Dummy frame to fill entire LLC
    if (use_facade) {
        read_frame_facade(frame_noise, &cache_stats_black);
    } else {
        read_frame(frame_noise, &cache_stats_black);
    }
    cache_stats_noise.llc_walk_time = read_frame_backwards(buffer_frame, &cache_stats_noise);
    sim_finish(&cache_stats_noise);

    // print_cache_stats(&cache_stats_noise);

    stats->uncompressed.num_evictions = cache_stats_noise.num_evictions;
    stats->uncompressed.walk_time = cache_stats_noise.llc_walk_time;

    // printf("TEXTURE SIZE: %ldKB\n", num_windows*2*WINDOW_SIZE_COMPRESSED / 1024);
    // printf("--------------------------------\n");
    // printf("BLACK LLC TIME\n");
    // print_cache_stats(&cache_stats_black);
    // printf("\n");
    // printf("NOISE LLC TIME\n");
    // print_cache_stats(&cache_stats_noise);
    // printf("\n");

    free_frames();
}

void print_stats_csv(llc_walk_stats_combined_t* stats, uint64_t num_stats) {
    printf("Texture Size\tLLC Walk Time (Uncompressed)\tLLC Walk Time (Compressed)\n");
    for (uint64_t i = 0; i < num_stats; i++) {
        printf("%" PRIu64 "\t%.2f\t%.2f\n", 
            stats[i].texture_size,
            stats[i].uncompressed.walk_time / 1000, 
            stats[i].compressed.walk_time / 1000);
    }
}

int main() {
    init_cache_config();
    srand(time(NULL));

    uint64_t lowBoundKB = 1;
    uint64_t upBoundKB = 128;
    uint64_t strideKB = 1;
    uint64_t num_iter = (upBoundKB - lowBoundKB) / strideKB + 1;

    llc_walk_stats_combined_t* stats = new llc_walk_stats_combined_t[num_iter];
    for (uint64_t i = 0; i < num_iter; i++) {
        uint64_t nKB = lowBoundKB + i*strideKB;
        if (nKB > upBoundKB) {
            nKB = upBoundKB;
        }
        stats[i].texture_size = nKB;
        measure_llc_walk(8*nKB, &stats[i], true);
    }

    print_stats_csv(stats, num_iter);

    delete[] stats;
    
    //Populate pixels
    // frame_t frame = get_new_frame();

    // uint64_t correct_pixels_base = 0;
    // uint64_t correct_pixels_facade = 0;
    // for (size_t i = 0; i < FRAME_SIZE; i++) {
    //     frame_t attack_frame = get_attacker_frame(frame.pixels[i]);
    //     double render_time_base = render_baseline(&attack_frame);
    //     double render_time_facade = render_facade(&attack_frame);

    //     bool correct_baseline = correct_guess(render_time_base > TIMING_THRESHOLD_BASELINE, frame.pixels[i]);
    //     correct_pixels_base += correct_baseline ? 1 : 0;
    //     bool correct_facade = correct_guess(render_time_facade > TIMING_THRESHOLD_FACADE, frame.pixels[i]);
    //     correct_pixels_facade += correct_facade ? 1 : 0;

    //     stats_baseline.avg_render_time = (stats_baseline.avg_render_time * i + render_time_base) / (i+1);
    //     stats_facade.avg_render_time = (stats_facade.avg_render_time * i + render_time_facade) / (i+1);
    // }

    // stats_baseline.accuracy = (double) (correct_pixels_base) / FRAME_SIZE;
    // stats_facade.accuracy = (double) (correct_pixels_facade) / FRAME_SIZE;

    //TEARDOWN

    // printf("BASELINE STATS\n");
    // print_stats(&stats_baseline);

    // printf("FACADE STATS\n");
    // print_stats(&stats_facade);


    // for (size_t i = 0; i < 32; i++) {
    //     for (size_t c = 0; c < 4; c++) {
    //         black.pixels[i][c] = 0;
    //         random.pixels[i][c] = rand() % 256;
    //     }
    // }

    // compress_result_t black_result = compress(&black);
    // compress_result_t random_result = compress(&random);

    // print_compress_result("BLACK", &black_result);
    // print_compress_result("RANDOM", &random_result);
}