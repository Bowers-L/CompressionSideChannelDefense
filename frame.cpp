#include "frame.hpp"
#include "cache_sim.hpp"

#include <stdlib.h>
#include <iostream>

std::vector<frame_t*> m_Frames;

void print_frame_nWindows() {
    for (uint64_t i = 0; i < m_Frames.size(); i++) {
        printf("m_Frames[%" PRIu64 "]->nWindows: %" PRIu64 "\n", i, (m_Frames[i])->nWindows);
    }
}

static uint64_t get_line_addr(uint64_t frame_id, uint64_t window_id) {
    const uint64_t WINDOW_SIZE = (2*WINDOW_SIZE_COMPRESSED);

    uint64_t line_offset = 0;

    for (uint64_t i = 0; i < frame_id; i++) {
        line_offset += ((m_Frames[i])->nWindows * WINDOW_SIZE);
    }

    return line_offset + window_id*WINDOW_SIZE;
}

static void set_pixel(uint8_t* p, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

static frame_t* init_frame(size_t nWindows) {
    frame_t* frame = new frame_t();
    frame->nWindows = nWindows;
    frame->windows = new pixel_window_t[nWindows];

    m_Frames.push_back(frame);
    return frame;
}

static bool get_frame_id(frame_t* searchFrame, uint64_t* id) {
    for (uint64_t i = 0; i < m_Frames.size(); i++) {
        if (searchFrame == m_Frames[i]) {
            *id = i;
            return true;
        }
    }

    return false;
}

void free_frame(frame_t* frame) {
    uint64_t frame_id;
    if (get_frame_id(frame, &frame_id)) {
        m_Frames.erase(m_Frames.begin() + frame_id);
    }
    delete[] frame->windows;
    delete frame;
}

static double read_window(frame_t* frame, uint64_t frame_id, uint64_t window_id, sim_stats_t* cache_stats) {
    pixel_window_t* window = &(frame->windows[window_id]);

    //Compress the window
    compress_result_t compress_result = compress(window);

    //Get the cache lines accessed
    uint64_t line;
    line = get_line_addr(frame_id, window_id);

    //Access the cachelines via the cache simulator
    double totalTime = sim_access('R', line, cache_stats);

    if (!compress_result.did_compression) {            
        totalTime += sim_access('R', line+WINDOW_SIZE_COMPRESSED, cache_stats);
    }

    return totalTime;
}

//Constructs a 128x128 pixel frame split into 512 windows, which uncompressed is enough to fill a 64KB cache
frame_t* get_new_frame_checkerboard(uint64_t nWindows) {
    frame_t* frame = init_frame(nWindows);

    //Checkerboard Pattern
    for (uint64_t i = 0; i < nWindows; i++) {
        for (size_t j = 0; j < WINDOW_NUM_PIXELS; j++) {
            if (i % 2 == 0) {
                set_pixel(frame->windows[i].pixels[j], 0, 0, 0, 0);
            } else {
                set_pixel(frame->windows[i].pixels[j], 255, 255, 255, 255);
            }
        }
    }

    return frame;
}

frame_t* get_new_frame_black(uint64_t nWindows) {
    frame_t* frame = init_frame(nWindows);

    for (uint64_t i = 0; i < nWindows; i++) {
        for (size_t j = 0; j < WINDOW_NUM_PIXELS; j++) {
            set_pixel(frame->windows[i].pixels[j], 0, 0, 0, 0);
        }
    }

    return frame;
}

frame_t* get_new_frame_random(uint64_t nWindows) {
    frame_t* frame = init_frame(nWindows);

    for (uint64_t i = 0; i < nWindows; i++) {
        for (size_t j = 0; j < WINDOW_NUM_PIXELS; j++) {
            set_pixel(frame->windows[i].pixels[j], rand() % 256, rand() % 256, rand() % 256, rand() % 256);
        }
    }

    return frame;
}

//returns the time required to read the frame
double read_frame(frame_t* frame, sim_stats_t* cache_stats) {
    double totalTime = 0.0;
    uint64_t frame_id;
    get_frame_id(frame, &frame_id);
    for (uint64_t i = 0; i < frame->nWindows; i++) {
        totalTime += read_window(frame, frame_id, i, cache_stats);
    }

    return totalTime;
}

double read_frame_backwards(frame_t* frame, sim_stats_t* cache_stats) {
    double totalTime = 0.0;
    uint64_t frame_id;
    get_frame_id(frame, &frame_id);
    for (uint64_t i = frame->nWindows-1; i >= 0; i--) {
        totalTime += read_window(frame, frame_id, i, cache_stats);

        if (i == 0) {
            break;
        }
    }

    return totalTime;
}

// static double render_baseline(frame_t* frame) {
//     double total = 0;
//     for (size_t row_start = 0; row_start < FRAME_NUM_ROWS; row_start += WINDOW_NUM_ROWS) {
//         for (size_t col_start = 0; col_start < FRAME_ROW_SIZE; col_start += WINDOW_ROW_SIZE) {
//             pixel_window_t frame_window;
//             for (size_t i = 0; i < WINDOW_NUM_ROWS; i++) {
//                 //rows
//                 for (size_t j = 0; j < WINDOW_ROW_SIZE; j++) {
//                     //cols
//                     for (size_t c = 0; c < 4; c++) {
//                         frame_window.pixels[i*WINDOW_ROW_SIZE+j][c] = 
//                             frame->pixels[(row_start+i)*FRAME_ROW_SIZE+(col_start+j)][c];
//                     }
//                 }
//             }

//             compress_result_t result = compress(&frame_window);
//             total += result.llc_time;
//         }
//     }

//     return total;
// }

// static double render_facade(frame_t* frame) {
//     frame_t facade;
//     uint8_t black[] = {0, 0, 0, 0};
//     uint8_t random[] = {(uint8_t) (rand() % 256), 
//                         (uint8_t) (rand() % 256), 
//                         (uint8_t) (rand() % 256), 
//                         (uint8_t) (rand() % 256)};
//     if (frame->pixels[0][0] > 128) {

//         facade = get_attacker_frame(black);
//     } else {
//         facade = get_attacker_frame(random);
//     }

//     double result_facade = render_baseline(&facade);
//     double result_base = render_baseline(frame);
//     return result_facade + result_base;
// }