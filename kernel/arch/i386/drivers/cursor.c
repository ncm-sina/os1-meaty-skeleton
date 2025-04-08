#include <kernel/drivers/cursor.h>
#include <kernel/vga_basic.h>

static cursor_state_t _cursor_state = {
    .x = 0,
    .y = 0,
    .type = CURSOR_DEFAULT,           // Default cursor type
    .visible = 0,
    .fg_color = VGA_COLOR_WHITE,      // Used only for non-default types
    .bg_color = VGA_COLOR_BLACK,      // Used only for non-default types
    .last_char = ' ',
    .last_attr = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREY
};

// Swap fg and bg colors of an attribute byte
static uint8_t swap_colors(uint8_t attr) {
    uint8_t fg = attr & 0x0F;       // Extract foreground (lower 4 bits)
    uint8_t bg = (attr >> 4) & 0x0F; // Extract background (upper 4 bits)
    return (fg << 4) | bg;           // Swap them
}

static void cursor_init(uint32_t width, uint32_t height) {
    _cursor_state.x = width / 2;
    _cursor_state.y = height / 2;
    _cursor_state.type = CURSOR_DEFAULT;
    _cursor_state.visible = 0;
    _cursor_state.fg_color = VGA_COLOR_WHITE;
    _cursor_state.bg_color = VGA_COLOR_BLACK;
    _vgab_get_char_at(_cursor_state.x, _cursor_state.y, &_cursor_state.last_char, &_cursor_state.last_attr);
}

static void cursor_show(void) {
    if (_cursor_state.visible) return;
    _vgab_get_char_at(_cursor_state.x, _cursor_state.y, &_cursor_state.last_char, &_cursor_state.last_attr);
    if (_cursor_state.type == CURSOR_DEFAULT) {
        uint8_t flipped_attr = swap_colors(_cursor_state.last_attr);
        _vgab_put_char_at(_cursor_state.x, _cursor_state.y, _cursor_state.last_char, flipped_attr);
    } else {
        uint8_t attrib = (_cursor_state.bg_color << 4) | _cursor_state.fg_color;
        _vgab_put_char_at(_cursor_state.x, _cursor_state.y, _cursor_state.type, attrib);
    }
    _cursor_state.visible = 1;
}

static void cursor_hide(void) {
    if (!_cursor_state.visible) return;
    _vgab_put_char_at(_cursor_state.x, _cursor_state.y, _cursor_state.last_char, _cursor_state.last_attr);
    _cursor_state.visible = 0;
}

static void cursor_move(int32_t x, int32_t y) {
    if (_cursor_state.visible) {
        // Restore previous cell
        _vgab_put_char_at(_cursor_state.x, _cursor_state.y, _cursor_state.last_char, _cursor_state.last_attr);

        // Save new cell's original state
        _vgab_get_char_at(x, y, &_cursor_state.last_char, &_cursor_state.last_attr);

        // Draw cursor at new position
        if (_cursor_state.type == CURSOR_DEFAULT) {
            uint8_t flipped_attr = swap_colors(_cursor_state.last_attr);
            _vgab_put_char_at(x, y, _cursor_state.last_char, flipped_attr);
        } else {
            uint8_t attrib = (_cursor_state.bg_color << 4) | _cursor_state.fg_color;
            _vgab_put_char_at(x, y, _cursor_state.type, attrib);
        }
    }
    _cursor_state.x = x;
    _cursor_state.y = y;
}

static void cursor_set_type(cursor_type_t type) {
    _cursor_state.type = type;
    if (_cursor_state.visible) {
        if (type == CURSOR_DEFAULT) {
            uint8_t flipped_attr = swap_colors(_cursor_state.last_attr);
            _vgab_put_char_at(_cursor_state.x, _cursor_state.y, _cursor_state.last_char, flipped_attr);
        } else {
            uint8_t attrib = (_cursor_state.bg_color << 4) | _cursor_state.fg_color;
            _vgab_put_char_at(_cursor_state.x, _cursor_state.y, type, attrib);
        }
    }
}

static void cursor_set_colors(uint8_t fg, uint8_t bg) {
    _cursor_state.fg_color = fg;
    _cursor_state.bg_color = bg;
    if (_cursor_state.visible && _cursor_state.type != CURSOR_DEFAULT) {
        uint8_t attrib = (bg << 4) | fg;
        _vgab_put_char_at(_cursor_state.x, _cursor_state.y, _cursor_state.type, attrib);
    }
}

struct cursor_driver cursor_drv = {
    .init = cursor_init,
    .show = cursor_show,
    .hide = cursor_hide,
    .move = cursor_move,
    .set_type = cursor_set_type,
    .set_colors = cursor_set_colors
};