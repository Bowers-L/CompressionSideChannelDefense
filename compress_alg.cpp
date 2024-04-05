#include "compress_alg.hpp"

#include <iostream>
#include <random>

#define COMPRESS_THRESHOLD 14

#define LLC_TIME_MEAN_COMPRESSED 16
#define LLC_TIME_MEAN_UNCOMPRESSED 18

std::normal_distribution<double> times_compressed(LLC_TIME_MEAN_COMPRESSED);
std::normal_distribution<double> times_uncompressed(LLC_TIME_MEAN_UNCOMPRESSED);

std::random_device rd{};
std::mt19937 gen{rd()};

//Returns -1 if value is 0, otherwise most significant bit position.
static int8_t get_msb_pos(uint8_t value) {
    int8_t msb_pos = 0;
    while (value != 0) {
        value >>= 1;
        msb_pos++;
    }

    return msb_pos-1;
}

static double get_render_time_ms(bool is_compressed) {
    if (is_compressed) {
        return times_compressed(gen);
    } else {
        return times_uncompressed(gen);
    }
}

//Returns if compression was applied to 
compress_result_t compress(pixel_window_t* window) {
    compress_result_t result;

    //Get Predictions
    uint8_t min[] = { 255, 255, 255, 255};
    uint8_t max[] = { 0, 0, 0, 0};
    for (size_t i = 0; i < WINDOW_SIZE; i++) {
        //printf("Pixel: (%d, %d, %d, %d)\n", window->pixels[i][0], window->pixels[i][1], window->pixels[i][2], window->pixels[i][3]);
        for (size_t c = 0; c < 4; c++) {
            if (window->pixels[i][c] < min[c]) {
                min[c] = window->pixels[i][c];
            }
            if (window->pixels[i][c] > max[c]) {
                max[c] = window->pixels[i][c];
            }
        }
    }

    //printf("Min: (%d, %d, %d, %d)\n", min[0], min[1], min[2], min[3]);
    //printf("Max: (%d, %d, %d, %d)\n", max[0], max[1], max[2], max[3]);


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

        //Need this in order to do the ceiling.
        if ((1 << l[c]) < diff[c]) {
            l[c]++;
        }
    }

    printf("Diff: (%d, %d, %d, %d)\n", diff[0], diff[1], diff[2], diff[3]);
    printf("L: (%d, %d, %d, %d)\n", l[0], l[1], l[2], l[3]);



    if (l[0] + l[1] + l[2] + l[3] <= COMPRESS_THRESHOLD) {
        result.did_compression = true;

        //This is always 0.5 because even when it's possible to compress less, the cacheline is padded with 0's
        result.compress_ratio = 0.5;
    } else {
        result.did_compression = false;
        result.compress_ratio = 1.0;
    }
    result.llc_time = get_render_time_ms(result.did_compression);


    //This is mostly for testing, the most important metric is whether the compression
    //was performed or not.
    //Note that in uncompressed formats these are not actually stored.
    for (size_t c = 0; c < 4; c++) {
        result.skip[c] =  l[c] == 0 ? (uint8_t) 1 : (uint8_t) 0; 
        result.prediction[c] = min[c];
        result.numBits[c] = l[c]; //Might be edge case with l[c] = 0 when diff is 1.
    }
    return result;
}