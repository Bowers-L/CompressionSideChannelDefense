#include <inttypes.h>
#include <stddef.h>

typedef struct {
    uint8_t pixels[32][4];
} pixel_window_t;

typedef struct {
    bool did_compression;
    double compress_ratio;

    //Compressed Pixels
    //Total Header Bits: (1+8+3) * 4 = 48

    //HEADER
    uint8_t skip[4];    //1 bit per channel
    uint8_t prediction[4];  //8 bits per channel

    //l[c] has a max value of 8, since 2^8 = 256 and the total possible difference is 255
    //Therefore l[c]-1 is stored to reduce the bits to 3.
    uint8_t numBits[4]; //3 bits per channel

    //Pixels: Max 14 bits allowed per pixel (3.5 per channel)
    //uint8_t pixels[32][4];
} compress_result_t;

extern compress_result_t compress(pixel_window_t* window);