#ifndef VBE_H
#define VBE_H

#include <stdint.h>

typedef struct {
    char signature[4];
    uint16_t version;
    uint32_t oem_string;
    uint32_t capabilities;
    uint32_t video_modes;
    uint16_t total_memory;
    uint16_t software_rev;
    uint32_t vendor;
    uint32_t product_name;
    uint32_t product_rev;
    uint8_t reserved[222];
    uint8_t oem_data[256];
} __attribute__((packed)) vbe_info_block_t;

typedef struct {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

typedef struct {
    uint32_t ax;
    uint32_t bx;
    uint32_t cx;
    uint32_t dx;
    uint32_t es;
    uint32_t di;
} __attribute__((packed)) vbe_bios_call_t;

void print_vbe_info_block(vbe_info_block_t *info);
void print_vbe_mode_info(vbe_mode_info_t *mode);

int vbe_list_supported_modes(void);
int32_t vbe_init(void);
uint32_t vbe_set_mode(uint16_t width, uint16_t height, uint8_t bpp);
uint32_t vbe_get_mode_info(uint16_t mode, vbe_mode_info_t * info);
int vbe_clear_screen(uint32_t color, vbe_mode_info_t *mode_info);
int vbe_set_text_mode(void);
int vbe_is_text_mode(void);

// Map the framebuffer to a virtual address
// void vbe_map_framebuffer(void);


// // Draw a pixel at (x, y)
// void vbe_draw_pixel(uint16_t x, uint16_t y, uint32_t color);

// // Fill a rectangle at (x, y) with width and height
// void vbe_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

#endif