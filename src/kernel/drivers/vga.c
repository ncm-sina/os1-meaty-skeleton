#include <kernel/drivers/vga.h>
#include <kernel/mport.h>

// VGA memory address (volatile struct)
static volatile VGA_MEMORY_ENTRY *const VGA_MEMORY = (volatile VGA_MEMORY_ENTRY *)0xB8000;

// VGA text info instance
VGA_TEXTINFO vga_textinfo = {
    .attribute = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREY, // Light gray on black
    .normattr = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREY,  // Same as default
    .screenheight = 25,                                         // Standard VGA height
    .screenwidth = 80,                                          // Standard VGA width
    .curx = 0,                                                  // Initial x position
    .cury = 0,                                                  // Initial y position
    .tabsize = 8                                                // Default tab size
};

void vga_get_char_at(int16_t x, int16_t y, uint8_t *ch, uint8_t *attrib){ // prints ch at x,y with attr attrib
    *ch = VGA_MEMORY[y * vga_textinfo.screenwidth + x].character;
    *attrib = VGA_MEMORY[y * vga_textinfo.screenwidth + x].attribute;
}

void vga_put_char_at(int16_t x, int16_t y, uint8_t ch, uint8_t attrib){
    VGA_MEMORY[y * vga_textinfo.screenwidth + x] = 
    (VGA_MEMORY_ENTRY){.character = ch, .attribute = attrib};
}

// Print a character at current cursor position using current attribute
void vga_put_char(char c) {
    if (vga_textinfo.curx >= vga_textinfo.screenwidth || vga_textinfo.cury >= vga_textinfo.screenheight) return;

    if (c == '\n') {
        vga_textinfo.curx = 0;
        vga_textinfo.cury++;
    } else if (c == '\t') {
        uint8_t spaces = vga_textinfo.tabsize - (vga_textinfo.curx % vga_textinfo.tabsize);
        for (uint8_t i = 0; i < spaces && vga_textinfo.curx < vga_textinfo.screenwidth; i++) {
            VGA_MEMORY[vga_textinfo.cury * vga_textinfo.screenwidth + vga_textinfo.curx] = 
                (VGA_MEMORY_ENTRY){.character = ' ', .attribute = vga_textinfo.attribute};
            vga_textinfo.curx++;
        }
    } else {
        VGA_MEMORY[vga_textinfo.cury * vga_textinfo.screenwidth + vga_textinfo.curx] = 
            (VGA_MEMORY_ENTRY){.character = c, .attribute = vga_textinfo.attribute};
        vga_textinfo.curx++;
    }

    if (vga_textinfo.curx >= vga_textinfo.screenwidth) {
        vga_textinfo.curx = 0;
        vga_textinfo.cury++;
    }
    if (vga_textinfo.cury >= vga_textinfo.screenheight) vga_scroll(1);
    vga_update_cursor();
}

// Print a string at current cursor position using current attribute
void vga_write_string(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_put_char(str[i]);
    }
}

// Scroll the screen up by 'step' lines
void vga_scroll(uint8_t step) {
    if (step >= vga_textinfo.screenheight) {
        vga_clear();
        return;
    }

    for (uint8_t y = step; y < vga_textinfo.screenheight; y++) {
        for (uint8_t x = 0; x < vga_textinfo.screenwidth; x++) {
            VGA_MEMORY[(y - step) * vga_textinfo.screenwidth + x] = 
                VGA_MEMORY[y * vga_textinfo.screenwidth + x];
        }
    }
    for (uint8_t y = vga_textinfo.screenheight - step; y < vga_textinfo.screenheight; y++) {
        for (uint8_t x = 0; x < vga_textinfo.screenwidth; x++) {
            VGA_MEMORY[y * vga_textinfo.screenwidth + x] = 
                (VGA_MEMORY_ENTRY){.character = ' ', .attribute = vga_textinfo.attribute};
        }
    }
    if (vga_textinfo.cury >= step) vga_textinfo.cury -= step;
    else vga_textinfo.cury = 0;
    vga_update_cursor();
}

// Set cursor position
void vga_gotoxy(uint8_t x, uint8_t y) {
    vga_textinfo.curx = x;
    vga_textinfo.cury = y;
    vga_update_cursor();
}

// Update hardware cursor
void vga_update_cursor(void) {
    uint16_t pos = vga_textinfo.cury * vga_textinfo.screenwidth + vga_textinfo.curx;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Hide the cursor
void vga_hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
}

// Show the cursor (for later use)
void vga_show_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x06); // Normal underscore cursor
}

// Get cursor position
CORDS vga_get_cursor(void) {
    return (CORDS){.x = vga_textinfo.curx, .y = vga_textinfo.cury};
}

// Reset text color to default
void vga_reset_textcolor(void) {
    vga_textinfo.attribute = vga_textinfo.normattr;
}

// Set text color (foreground and background)
void vga_set_textcolor(uint8_t fg, uint8_t bg) {
    vga_textinfo.attribute = (bg << 4) | (fg & 0x0F);
}

// Get current text color
unsigned char vga_get_textcolor(void) {
    return vga_textinfo.attribute;
}

// Clear the screen with the current background color
void vga_clear(void) {
    for (uint8_t y = 0; y < vga_textinfo.screenheight; y++) {
        for (uint8_t x = 0; x < vga_textinfo.screenwidth; x++) {
            VGA_MEMORY[y * vga_textinfo.screenwidth + x] = 
                (VGA_MEMORY_ENTRY){.character = ' ', .attribute = vga_textinfo.attribute};
        }
    }
    vga_textinfo.curx = 0;
    vga_textinfo.cury = 0;
    vga_update_cursor();
}

// Set tab size
void vga_set_tabsize(uint8_t size) {
    vga_textinfo.tabsize = size;
}

// Get tab size
uint8_t vga_get_tabsize(void) {
    return vga_textinfo.tabsize;
}