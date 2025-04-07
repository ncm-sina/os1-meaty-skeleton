#include <kernel/drivers/keyboard.h>
#include <kernel/isrs/keyboard.h>

#define KEYBOARD_BUFFER_SIZE 64

static struct key_event buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t buffer_head = 0;
static uint32_t buffer_tail = 0;
static uint8_t modifiers = 0;
static uint8_t extended = 0;

static enum keycode scancode_to_keycode(uint8_t scancode) {
    if (extended) {
        extended = 0;
        switch (scancode) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            default: return KEY_UNKNOWN;
        }
    }
    switch (scancode) {
        case 0x1E: return KEY_A;
        case 0x30: return KEY_B;
        case 0x2E: return KEY_C;
        case 0x3B: return KEY_F1;
        case 0x3C: return KEY_F2;
        case 0x3D: return KEY_F3;
        case 0x3E: return KEY_F4;
        case 0x3F: return KEY_F5;
        case 0x40: return KEY_F6;
        case 0x41: return KEY_F7;
        case 0x42: return KEY_F8;
        case 0x43: return KEY_F9;
        case 0x44: return KEY_F10;
        case 0x57: return KEY_F11;
        case 0x58: return KEY_F12;
        case 0x2A: return KEY_LSHIFT;
        case 0x36: return KEY_RSHIFT;
        case 0x1D: return KEY_LCTRL;
        case 0x38: return KEY_LALT;
        case 0x1C: return KEY_ENTER;
        case 0x39: return KEY_SPACE;
        case 0x0E: return KEY_BACKSPACE;
        default: return KEY_UNKNOWN;
    }
}

static void queue_event(enum keycode code, enum key_event_type type) {
    uint32_t next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head == buffer_tail) return;
    buffer[buffer_head].code = code;
    buffer[buffer_head].type = type;
    buffer[buffer_head].modifiers = modifiers;
    buffer_head = next_head;
}

static void keyboard_handle_scancode(uint8_t scancode) {
    enum keycode code;

    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    int released = scancode & 0x80;
    if (released) scancode &= 0x7F;
    code = scancode_to_keycode(scancode);

    if (code == KEY_UNKNOWN) return;

    switch (code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            if (released) modifiers &= ~MOD_SHIFT;
            else modifiers |= MOD_SHIFT;
            break;
        case KEY_LCTRL:
            if (released) modifiers &= ~MOD_CTRL;
            else modifiers |= MOD_CTRL;
            break;
        case KEY_LALT:
            if (released) modifiers &= ~MOD_ALT;
            else modifiers |= MOD_ALT;
            break;
        default:
            queue_event(code, released ? KEY_RELEASE : KEY_PRESS);
            break;
    }
}

static void keyboard_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
    modifiers = 0;
    extended = 0;
    isr_keyboard_register_handler(keyboard_handle_scancode);
}

static int keyboard_get_event(struct key_event *event) {
    if (buffer_head == buffer_tail) return 0;
    *event = buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return 1;
}

static uint8_t keyboard_get_modifiers(void) {
    return modifiers;
}

struct keyboard_driver keyboard_drv = {
    .init = keyboard_init,
    .get_event = keyboard_get_event,
    .get_modifiers = keyboard_get_modifiers
};