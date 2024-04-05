#include <iostream>
#include <stdlib.h>
#include "time.h"
#include "attacker.hpp"

#include "compress_alg.hpp"

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
    for (size_t i = 0; i < FRAME_SIZE*FRAME_SIZE; i++) {
        if (i % 2 == 0) {
            set_pixel(frame.pixels[i], 0, 0, 0, 0);
        } else {
            set_pixel(frame.pixels[i], 1, 1, 1, 1);
        }
    }

    return frame;
}

int main() {
    pixel_window_t black;
    pixel_window_t random;
    srand(time(NULL));
    
    //Populate pixels
    //frame_t frame = get_new_frame();

    for (size_t i = 0; i < 32; i++) {
        for (size_t c = 0; c < 4; c++) {
            black.pixels[i][c] = 0;
            random.pixels[i][c] = rand() % 256;
        }
    }

    compress_result_t black_result = compress(&black);
    compress_result_t random_result = compress(&random);

    print_compress_result("BLACK", &black_result);
    print_compress_result("RANDOM", &random_result);
}