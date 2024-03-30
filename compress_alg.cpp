#include "compress_alg.hpp"

#define COMPRESS_THRESHOLD 14

//Returns -1 if value is 0, otherwise most significant bit position.
static int8_t get_msb_pos(uint8_t value) {
    int8_t msb_pos = 0;
    while (value != 0) {
        value >>= 1;
        msb_pos++;
    }

    return msb_pos-1;
}

//Returns if compression was applied to 
compress_result_t compress(pixel_window_t* window) {
    compress_result_t result;

    //Get Predictions
    uint8_t min[] = { 255, 255, 255, 255};
    uint8_t max[] = { 0, 0, 0, 0};
    for (size_t i = 0; i < 32; i++) {
        for (size_t c = 0; c < 4; c++) {
            if (window->pixels[i][c] < min[c]) {
                min[c] = window->pixels[i][c];
            }
            if (window->pixels[i][c] > max[c]) {
                max[c] = window->pixels[i][c];
            }
        }
    }

    uint8_t diff[4];
    uint8_t l[4];
    for (size_t c = 0; c < 4; c++) {
        diff[c] = max[c] - min[c];

        //l[c] = ceil(log(max[c] - min[c]))
        int8_t msb = get_msb_pos(diff[c]);
        if (msb < 0) {
            l[c] = 0;
        } else {
            l[c] = (uint8_t) msb;
        }

        if (l[c] > diff[c]) {
            l[c]++;
        }
    }

    if (l[0] + l[1] + l[2] + l[3] <= COMPRESS_THRESHOLD) {
        //actually do compression
    } else {
        result.did_compression = false;
        result.compress_ratio = 1.0;
    }
    
    return result;
}