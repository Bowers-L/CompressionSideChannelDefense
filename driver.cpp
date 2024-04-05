#include <iostream>
#include <stdlib.h>
#include "time.h"
#include "attacker.hpp"

#include "compress_alg.hpp"

#define ATTACK_THRESHOLD 17

static std::string channel_to_str[] = {
    "Red", "Green", "Blue", "Alpha"
};

static std::string bool_to_string(bool value) {
    return value ? "Yes" : "No";
}

void print_compress_result(std::string name, const compress_result_t* result) {
    printf("Compressed pattern: %s\n", name.c_str());
    printf("\tDid Compression: %s\n", bool_to_string(result->did_compression).c_str());
    printf("\tCompression Ratio: %.2f\n", result->compress_ratio);
    printf("\tLLC Time: %.2f\n", result->llc_time);
    for (size_t c = 0; c < 4; c++) {
        printf("\t%s Channel\n", channel_to_str[c].c_str());
        printf("\t\tSkip Bit: %d\n", result->skip[c]);
        printf("\t\t Prediction: %d\n", result->prediction[c]);
        printf("\t\t NumBits: %d\n", result->numBits[c]);
    }
    printf("\n");
}

void set_pixel(uint8_t* p, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

frame_t get_new_frame() {
    frame_t frame;

    //Checkerboard Pattern
    for (size_t i = 0; i < FRAME_SIZE; i++) {
        if (i % 2 == 0) {
            set_pixel(frame.pixels[i], 0, 0, 0, 0);
        } else {
            set_pixel(frame.pixels[i], 255, 255, 255, 255);
        }
    }

    return frame;
}

frame_t get_attacker_frame(uint8_t* pixel) {
    frame_t frame;

    //Use red channel to determine attacker pixel
    if (pixel[0] > 128) {
        for (size_t i = 0; i < FRAME_SIZE; i++) {
            set_pixel(frame.pixels[i], rand() % 256, rand() % 256, rand() % 256, rand() % 256);
        }
    } else {
        for (size_t i = 0; i < FRAME_SIZE; i++) {
            set_pixel(frame.pixels[i], 0, 0, 0, 0);
        }
    }

    return frame;
}

double get_total_render_time(frame_t* frame) {
    double total = 0;
    for (size_t row_start = 0; row_start < FRAME_NUM_ROWS; row_start += WINDOW_NUM_ROWS) {
        for (size_t col_start = 0; col_start < FRAME_ROW_SIZE; col_start += WINDOW_ROW_SIZE) {
            pixel_window_t frame_window;
            for (size_t i = 0; i < WINDOW_NUM_ROWS; i++) {
                //rows
                for (size_t j = 0; j < WINDOW_ROW_SIZE; j++) {
                    //cols
                    for (size_t c = 0; c < 4; c++) {
                        frame_window.pixels[i*WINDOW_ROW_SIZE+j][c] = 
                            frame->pixels[(row_start+i)*FRAME_ROW_SIZE+(col_start+j)][c];
                    }
                }
            }

            compress_result_t result = compress(&frame_window);
            total += result.llc_time;
        }
    }

    return total;
}

int main() {
    // pixel_window_t black;
    // pixel_window_t random;
    srand(time(NULL));
    
    //Populate pixels
    frame_t frame = get_new_frame();

    for (size_t i = 0; i < 2; i++) {
        frame_t attack_frame = get_attacker_frame(frame.pixels[i]);
        double render_time = get_total_render_time(&attack_frame);
        printf("Render Time: %.2f\n", render_time);
    }

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