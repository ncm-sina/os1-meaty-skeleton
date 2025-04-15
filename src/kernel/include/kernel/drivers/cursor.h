#ifndef DRIVERS_CURSOR_H
#define DRIVERS_CURSOR_H

#include <stdint.h>

// Cursor types (0 is default)
typedef enum {
    CURSOR_DEFAULT = 0,      // Inverts fg/bg colors, no character
    CURSOR_ARROW = '^',      // Up arrow
    CURSOR_BLOCK = 219,      // Full block
    CURSOR_UNDERLINE = '_',  // Underscore
    CURSOR_CROSS = '+'       // Crosshair
} cursor_type_t;

typedef struct {
    int32_t x;           // Current X position
    int32_t y;           // Current Y position
    cursor_type_t type;  // Cursor appearance
    uint8_t visible;     // 0 = hidden, 1 = visible
    uint8_t fg_color;    // Foreground color (VGA attribute)
    uint8_t bg_color;    // Background color (VGA attribute)
    uint8_t last_char;   // Last character under cursor
    uint8_t last_attr;   // Last attribute under cursor
} cursor_state_t;

struct cursor_driver {
    void (*init)(uint32_t width, uint32_t height); // Initialize cursor with screen size
    void (*show)(void);                            // Show cursor
    void (*hide)(void);                            // Hide cursor
    void (*move)(int32_t x, int32_t y);            // Move cursor to position
    void (*set_type)(cursor_type_t type);          // Set cursor appearance
    void (*set_colors)(uint8_t fg, uint8_t bg);    // Set fg/bg colors (for non-default types)
};

extern struct cursor_driver cursor_drv;

#endif