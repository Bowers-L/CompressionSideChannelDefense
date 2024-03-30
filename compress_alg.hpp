#include <inttypes.h>
#include <stddef.h>

typedef struct {
    uint8_t pixels[32][4];
} pixel_window_t;

typedef struct {
    bool did_compression;
    double compress_ratio;
} compress_result_t;

extern compress_result_t compress(pixel_window_t* window);