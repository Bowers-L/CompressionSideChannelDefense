#ifndef FRAME_HPP
#define FRAME_HPP

#include "cache_sim.hpp"
#include "compress_alg.hpp"

#include <vector>

#define FRAME_NUM_WINDOWS_CACHE 512 //Equal to NUM_CACHE_LINES / 2
// #define FRAME_ROW_SIZE 32
// #define FRAME_NUM_ROWS ((FRAME_SIZE) / (FRAME_ROW_SIZE))
// #define FRAME_NUM_WINDOWS ((FRAME_SIZE) / 16)   //16 pixels per cacheline

//A frame is a texture that is designed to occupy the whole LLC.
typedef struct {
    uint64_t nWindows;
    pixel_window_t* windows;
} frame_t;

extern std::vector<frame_t*> m_Frames;

extern void print_frame_nWindows();
extern double read_frame(frame_t* frame, sim_stats_t* cache_stats);
extern double read_frame_backwards(frame_t* frame, sim_stats_t* cache_stats);
extern frame_t* get_new_frame_checkerboard(uint64_t nWindows);
extern frame_t* get_new_frame_black(uint64_t nWindows);
extern frame_t* get_new_frame_random(uint64_t nWindows);
extern void free_frame(frame_t* frame);

#endif