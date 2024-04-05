#include <inttypes.h>
#include <stddef.h>

#define FRAME_SIZE 1024
#define FRAME_ROW_SIZE 32
#define FRAME_NUM_ROWS ((FRAME_SIZE) / (FRAME_ROW_SIZE))


#define WINDOW_SIZE 32
#define WINDOW_ROW_SIZE 8
#define WINDOW_NUM_ROWS ((WINDOW_SIZE) / (WINDOW_ROW_SIZE))

#define NUM_CHANNELS 4

typedef struct {
    uint8_t pixels[WINDOW_SIZE][NUM_CHANNELS];
} pixel_window_t;

typedef struct {
    uint8_t pixels[FRAME_SIZE][NUM_CHANNELS];
} frame_t;

typedef struct {
    bool did_compression;
    double compress_ratio;
    double llc_time;

    //Compressed Pixels
    //Total Header Bits: (1+8+3) * 4 = 48

    //HEADER
    uint8_t skip[NUM_CHANNELS];    //1 bit per channel
    uint8_t prediction[NUM_CHANNELS];  //8 bits per channel

    //l[c] has a max value of 8, since 2^8 = 256 and the total possible difference is 255
    //Therefore l[c]-1 is stored to reduce the bits to 3.
    uint8_t numBits[NUM_CHANNELS]; //3 bits per channel

    //Pixels: Max 14 bits allowed per pixel (3.5 per channel)
    //uint8_t pixels[32][4];
} compress_result_t;

extern compress_result_t compress(pixel_window_t* window);