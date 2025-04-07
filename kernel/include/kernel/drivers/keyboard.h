#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#include <stdint.h>

enum keycode {
    KEY_UNKNOWN = 0,
    KEY_A = 0x1E, KEY_B = 0x30, KEY_C = 0x2E,
    KEY_F1 = 0x3B, KEY_F2 = 0x3C, KEY_F3 = 0x3D, KEY_F4 = 0x3E,
    KEY_F5 = 0x3F, KEY_F6 = 0x40, KEY_F7 = 0x41, KEY_F8 = 0x42,
    KEY_F9 = 0x43, KEY_F10 = 0x44, KEY_F11 = 0x57, KEY_F12 = 0x58,
    KEY_LSHIFT = 0x2A, KEY_RSHIFT = 0x36,
    KEY_LCTRL = 0x1D, KEY_LALT = 0x38,
    KEY_UP = 0xE048, KEY_DOWN = 0xE050,
    KEY_LEFT = 0xE04B, KEY_RIGHT = 0xE04D,
    KEY_ENTER = 0x1C, KEY_SPACE = 0x39, KEY_BACKSPACE = 0x0E
};

enum key_event_type {
    KEY_PRESS,
    KEY_RELEASE
};

struct key_event {
    enum keycode code;
    enum key_event_type type;
    uint8_t modifiers;
};

#define MOD_SHIFT  (1 << 0)
#define MOD_CTRL   (1 << 1)
#define MOD_ALT    (1 << 2)

struct keyboard_driver {
    void (*init)(void);
    int (*get_event)(struct key_event *event);
    uint8_t (*get_modifiers)(void);
};

extern struct keyboard_driver keyboard_drv;

#endif