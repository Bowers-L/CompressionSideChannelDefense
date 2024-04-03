#include <iostream>
#include <stdlib.h>
#include "time.h"

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
    for (size_t c = 0; c < 4; c++) {
        printf("\t%s Channel\n", channel_to_str[c].c_str());
        printf("\t\tSkip Bit: %d\n", result->skip[c]);
        printf("\t\t Prediction: %d\n", result->prediction[c]);
        printf("\t\t NumBits: %d\n", result->numBits[c]);
    }
    printf("\n");
}

int main() {
    pixel_window_t black;
    pixel_window_t random;
    srand(time(NULL));
    
    //Populate pixels

    //Black Pattern
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