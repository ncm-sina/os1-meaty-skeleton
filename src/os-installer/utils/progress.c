#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "progress.h"
#include <kernel/drivers/vga.h>
#include <stdio.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint32_t progress_max = 0;
static uint32_t progress_current = 0;
static uint8_t progress_width = 0;
static uint8_t progress_x = 0;
static uint8_t progress_y = 0;

/* Initialize progress bar */
void init_progress(uint32_t max_value, uint8_t width) {
    progress_max = max_value;
    progress_current = 0;
    progress_width = width;
    progress_x = (VGA_WIDTH - width) / 2; /* Center bar */
    progress_y = VGA_HEIGHT - 3; /* Place above action bar */
    update_progress(0);
}

/* Update progress bar */
void update_progress(uint32_t value) {
    progress_current = value > progress_max ? progress_max : value;
    float ratio = (float)progress_current / progress_max;
    uint32_t filled = (uint32_t)(ratio * progress_width);
    uint32_t empty = progress_width - filled;
    
    vga_gotoxy(progress_x, progress_y);
    vga_set_textcolor(VGA_COLOR_DARK_GREY, VGA_COLOR_WHITE);
    for (uint32_t i = 0; i < filled; i++) vga_put_char(' ');
    vga_set_textcolor(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
    for (uint32_t i = 0; i < empty; i++) vga_put_char(' ');
    
    vga_gotoxy(progress_x + progress_width + 2, progress_y);
    printf("%d%%", (int)(ratio * 100));
}

/* Get current progress */
uint32_t get_progress(void) {
    return progress_current;
}
