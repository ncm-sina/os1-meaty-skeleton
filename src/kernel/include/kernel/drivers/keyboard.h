#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#include <stdint.h>

// Keycode enum representing all keys on a 101-key PS/2 keyboard, including extended and multimedia keys
enum keycode {
    KEY_UNKNOWN = 0,
    // Letters (A-Z)
    KEY_A = 0x1E, KEY_B = 0x30, KEY_C = 0x2E, KEY_D = 0x20, KEY_E = 0x12,
    KEY_F = 0x21, KEY_G = 0x22, KEY_H = 0x23, KEY_I = 0x17, KEY_J = 0x24,
    KEY_K = 0x25, KEY_L = 0x26, KEY_M = 0x32, KEY_N = 0x31, KEY_O = 0x18,
    KEY_P = 0x19, KEY_Q = 0x10, KEY_R = 0x13, KEY_S = 0x1F, KEY_T = 0x14,
    KEY_U = 0x16, KEY_V = 0x2F, KEY_W = 0x11, KEY_X = 0x2D, KEY_Y = 0x15,
    KEY_Z = 0x2C,
    // Numbers (0-9)
    KEY_0 = 0x0B, KEY_1 = 0x02, KEY_2 = 0x03, KEY_3 = 0x04, KEY_4 = 0x05,
    KEY_5 = 0x06, KEY_6 = 0x07, KEY_7 = 0x08, KEY_8 = 0x09, KEY_9 = 0x0A,
    // Function keys (F1-F12)
    KEY_F1 = 0x3B, KEY_F2 = 0x3C, KEY_F3 = 0x3D, KEY_F4 = 0x3E,
    KEY_F5 = 0x3F, KEY_F6 = 0x40, KEY_F7 = 0x41, KEY_F8 = 0x42,
    KEY_F9 = 0x43, KEY_F10 = 0x44, KEY_F11 = 0x57, KEY_F12 = 0x58,
    // Modifiers
    KEY_LSHIFT = 0x2A, KEY_RSHIFT = 0x36, KEY_LCTRL = 0x1D, KEY_LALT = 0x38,
    KEY_CAPSLOCK = 0x3A, KEY_NUMLOCK = 0x45,
    // Navigation and editing (extended)
    KEY_UP = 0xE048, KEY_DOWN = 0xE050, KEY_LEFT = 0xE04B, KEY_RIGHT = 0xE04D,
    KEY_HOME = 0xE047, KEY_END = 0xE04F, KEY_INSERT = 0xE052, KEY_DELETE = 0xE053,
    KEY_PAGEUP = 0xE049, KEY_PAGEDOWN = 0xE051,
    // Other common keys
    KEY_ENTER = 0x1C, KEY_SPACE = 0x39, KEY_BACKSPACE = 0x0E, KEY_TAB = 0x0F,
    KEY_ESC = 0x01, KEY_MINUS = 0x0C, KEY_EQUAL = 0x0D, KEY_LBRACKET = 0x1A,
    KEY_RBRACKET = 0x1B, KEY_SEMICOLON = 0x27, KEY_QUOTE = 0x28,
    KEY_BACKTICK = 0x29, KEY_COMMA = 0x33, KEY_PERIOD = 0x34, KEY_SLASH = 0x35,
    KEY_BACKSLASH = 0x2B,
    // Numpad (non-extended unless Num Lock off)
    KEY_NUMPAD0 = 0x52, KEY_NUMPAD1 = 0x4F, KEY_NUMPAD2 = 0x50, KEY_NUMPAD3 = 0x51,
    KEY_NUMPAD4 = 0x4B, KEY_NUMPAD5 = 0x4C, KEY_NUMPAD6 = 0x4D, KEY_NUMPAD7 = 0x47,
    KEY_NUMPAD8 = 0x48, KEY_NUMPAD9 = 0x49, KEY_NUMPAD_MULTIPLY = 0x37,
    KEY_NUMPAD_PLUS = 0x4E, KEY_NUMPAD_MINUS = 0x4A, KEY_NUMPAD_PERIOD = 0x53,
    KEY_NUMPAD_DIVIDE = 0xE035, KEY_NUMPAD_ENTER = 0xE01C,
    // Special keys (some extended)
    KEY_PRINTSCREEN = 0xE037, KEY_SCROLLLOCK = 0x46, KEY_PAUSE = 0xE045,
    // Multimedia keys (often extended or custom, simplified here)
    KEY_VOLUMEUP = 0xE030, KEY_VOLUMEDOWN = 0xE02E, KEY_MUTE = 0xE020,
    KEY_BRIGHTNESSUP = 0xE06F, KEY_BRIGHTNESSDOWN = 0xE06D
};

enum key_event_type {
    KEY_PRESS,
    KEY_RELEASE
};

// Structure to hold a key event (keycode, press/release, and modifier state)
struct key_event {
    enum keycode code;
    enum key_event_type type;
    uint8_t modifiers;
};

// Modifier flags (bitfield)
#define MOD_SHIFT    (1 << 0) // Left or Right Shift pressed
#define MOD_CTRL     (1 << 1) // Left Control pressed
#define MOD_ALT      (1 << 2) // Left Alt pressed
#define MOD_CAPSLOCK (1 << 3) // Caps Lock active
#define MOD_NUMLOCK  (1 << 4) // Num Lock active

// Keyboard driver interface
struct keyboard_driver {
    void (*init)(void);                  // Initialize driver and register with ISR
    int (*get_event)(struct key_event *event); // Get next key event from buffer
    uint8_t (*get_modifiers)(void);      // Get current modifier state
    char (*keycode_to_ascii)(enum keycode code, uint8_t modifiers); // Convert keycode to ASCII
    void (*clear_buffer)(void);          // Clear all events from the buffer
};

extern struct keyboard_driver keyboard_drv;

#endif