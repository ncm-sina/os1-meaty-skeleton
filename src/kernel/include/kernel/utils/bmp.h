#ifndef BMP_H
#define BMP_H

#include <stdint.h>

// BMP file header (14 bytes)
typedef struct {
    uint16_t signature; // 'BM' (0x4D42)
    uint32_t file_size; // Size of the BMP file
    uint32_t reserved;  // Unused
    uint32_t data_offset; // Offset to pixel data
} BmpFileHeader;

// BMP info header (BITMAPINFOHEADER, 40 bytes)
typedef struct {
    uint32_t header_size; // Size of this header (40)
    int32_t width;        // Image width in pixels
    int32_t height;       // Image height in pixels (positive = bottom-up, negative = top-down)
    uint16_t planes;      // Number of planes (1)
    uint16_t bpp;         // Bits per pixel (24 or 32)
    uint32_t compression; // Compression method (0 = none)
    uint32_t image_size;  // Size of raw bitmap data
    int32_t x_pixels_per_meter; // Horizontal resolution
    int32_t y_pixels_per_meter; // Vertical resolution
    uint32_t colors_used;       // Number of colors in palette (0 = default)
    uint32_t colors_important;  // Number of important colors (0 = all)
} BmpInfoHeader;

// 32-bit pixel format (RGBA)
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha; // May be unused in 24-bit BMP
} Pixel32;

// Reads a BMP image from a binary buffer and outputs a 32-bit RGBA pixel buffer.
// Returns 0 on success, non-zero on error.
// Output buffer is allocated by the caller and must be large enough (width * height * 4 bytes).
int read_bmp(const uint8_t *buffer, uint32_t buffer_size, Pixel32 *output, uint32_t *width, uint32_t *height);

#endif