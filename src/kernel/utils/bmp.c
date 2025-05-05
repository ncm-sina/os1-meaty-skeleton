#include <kernel/utils/bmp.h>

// Error codes
#define BMP_ERROR_INVALID_SIGNATURE 1
#define BMP_ERROR_INVALID_HEADER 2
#define BMP_ERROR_UNSUPPORTED_BPP 3
#define BMP_ERROR_BUFFER_TOO_SMALL 4
#define BMP_ERROR_INVALID_DIMENSIONS 5

int read_bmp(const uint8_t *buffer, uint32_t buffer_size, Pixel32 *output, uint32_t *width, uint32_t *height) {
    // Check buffer size for minimum header (14 + 40 bytes)
//     if(vbe_set_text_mode()) printf("5error setting text mode err\n");
//     printf("1buf:%08x ", buffer );
// while(1);
    if (buffer_size < 54) {
        return BMP_ERROR_BUFFER_TOO_SMALL;
    }

    // Read file header
    BmpFileHeader file_header;
    file_header.signature = *(uint16_t *)(buffer);
    file_header.file_size = *(uint32_t *)(buffer + 2);
    file_header.reserved = *(uint32_t *)(buffer + 6);
    file_header.data_offset = *(uint32_t *)(buffer + 10);

    // Validate signature ('BM')
    if (file_header.signature != 0x4D42) {
        return BMP_ERROR_INVALID_SIGNATURE;
    }

    // Read info header
    BmpInfoHeader info_header;
    info_header.header_size = *(uint32_t *)(buffer + 14);
    info_header.width = *(int32_t *)(buffer + 18);
    info_header.height = *(int32_t *)(buffer + 22);
    info_header.planes = *(uint16_t *)(buffer + 26);
    info_header.bpp = *(uint16_t *)(buffer + 28);
    info_header.compression = *(uint32_t *)(buffer + 30);
    info_header.image_size = *(uint32_t *)(buffer + 34);

    // Validate header size and dimensions
    if (info_header.header_size < 40 || info_header.width <= 0 || info_header.height == 0) {
        return BMP_ERROR_INVALID_HEADER;
    }

    // Check bits per pixel (24 or 32)
    if (info_header.bpp != 24 && info_header.bpp != 32) {
        return BMP_ERROR_UNSUPPORTED_BPP;
    }

    // Check compression (must be none)
    if (info_header.compression != 0) {
        return BMP_ERROR_INVALID_HEADER;
    }

    // Validate buffer size
    if (buffer_size < file_header.data_offset + info_header.image_size) {
        return BMP_ERROR_BUFFER_TOO_SMALL;
    }

    // Set output dimensions
    *width = info_header.width;
    *height = info_header.height > 0 ? info_header.height : -info_header.height;

    // Calculate row padding (rows are padded to 4-byte boundaries)
    uint32_t bytes_per_pixel = info_header.bpp / 8;
    uint32_t row_size = info_header.width * bytes_per_pixel;
    uint32_t padding = (4 - (row_size % 4)) % 4;
    uint32_t padded_row_size = row_size + padding;

    // Read pixel data
    const uint8_t *pixel_data = buffer + file_header.data_offset;
    uint32_t abs_height = *height;
    int32_t src_height = info_header.height;

    for (uint32_t y = 0; y < abs_height; y++) {
        // Handle bottom-up (positive height) or top-down (negative height)
        uint32_t src_y = (src_height > 0) ? (abs_height - 1 - y) : y;
        const uint8_t *row = pixel_data + src_y * padded_row_size;

        for (uint32_t x = 0; x < info_header.width; x++) {
            uint32_t idx = y * info_header.width + x;
            const uint8_t *pixel = row + x * bytes_per_pixel;

            // Read RGB (and alpha if 32-bit)
            output[idx].blue = pixel[0];
            output[idx].green = pixel[1];
            output[idx].red = pixel[2];
            output[idx].alpha = (bytes_per_pixel == 4) ? pixel[3] : 0xFF; // Opaque if 24-bit
        }
    }

    return 0;
}