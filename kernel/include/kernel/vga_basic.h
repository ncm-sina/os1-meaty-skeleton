#ifndef VGA_BASIC_H
#define VGA_BASIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// VGA color enum (16 colors for foreground and background)
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15
} VGA_COLORS;

// Cursor type enum (not used for now)
typedef enum {
    _NOCURSOR,      // No cursor
    _SOLIDCURSOR,   // Solid block cursor
    _NORMALCURSOR   // Normal underscore cursor
} VGA_CURSORTYPE;

// VGA text info struct
typedef struct {
    unsigned char attribute;      // Current text attribute (fg+bg)
    unsigned char normattr;       // Default text attribute (fg+bg)
    uint8_t screenheight;         // Text screen's height
    uint8_t screenwidth;          // Text screen's width
    uint8_t curx;                 // X-coordinate in current window
    uint8_t cury;                 // Y-coordinate in current window
    uint8_t tabsize;              // Tab size
} VGAB_TEXTINFO;

// VGA memory entry struct
typedef struct {
    unsigned char character;      // ASCII character
    unsigned char attribute;      // Color attribute (fg+bg)
} VGA_MEMORY_ENTRY;

typedef struct {
    uint8_t x;
    uint8_t y;
} CORDS;

// Function prototypes (all static, prefixed with _vgab_)
void _vgab_get_char_at(int16_t x, int16_t y, uint8_t *ch, uint8_t *attrib); // prints ch at x,y with attr attrib
void _vgab_put_char_at(int16_t x, int16_t y, uint8_t ch, uint8_t attrib); // prints ch at x,y with attr attrib
void _vgab_put_char(char c); // prints a character at curx, curx using current text attribute
void _vgab_write_string(const char *str); // prints a string at curx, cury using current text attribute
void _vgab_scroll(uint8_t step); // would scroll step lines
void _vgab_set_cursor(uint8_t x, uint8_t y);
void _vgab_update_cursor(void);
void _vgab_hide_cursor(void);
void _vgab_show_cursor(void);
CORDS _vgab_get_cursor(void);
void _vgab_reset_textcolor(); // resets current text attribute to default text attribute
void _vgab_set_textcolor(uint8_t fg, uint8_t bg);
unsigned char _vgab_get_textcolor(void);
void _vgab_clear_screen(void);
void _vgab_set_tabsize(uint8_t size);
uint8_t _vgab_get_tabsize(void);

#endif