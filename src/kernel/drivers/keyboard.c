#include <kernel/drivers/keyboard.h>
#include <kernel/arch/i386/isrs/keyboard.h>

#define KEYBOARD_BUFFER_SIZE 64 // Size of the circular buffer for key events

// Circular buffer to store key events
static struct key_event buffer[KEYBOARD_BUFFER_SIZE];
// Head: where new events are added; Tail: where events are removed
static uint32_t buffer_head = 0;
static uint32_t buffer_tail = 0;
// Current modifier state (Shift, Ctrl, Alt, Caps Lock, Num Lock)
static uint8_t modifiers = 0;
// Flag for extended scancodes (0xE0 prefix)
static uint8_t extended = 0;

// Lookup table for non-extended scancodes to keycodes (PS/2 Set 1)
static const enum keycode scancode_table[128] = {
    [0x01] = KEY_ESC, [0x02] = KEY_1, [0x03] = KEY_2, [0x04] = KEY_3,
    [0x05] = KEY_4, [0x06] = KEY_5, [0x07] = KEY_6, [0x08] = KEY_7,
    [0x09] = KEY_8, [0x0A] = KEY_9, [0x0B] = KEY_0, [0x0C] = KEY_MINUS,
    [0x0D] = KEY_EQUAL, [0x0E] = KEY_BACKSPACE, [0x0F] = KEY_TAB,
    [0x10] = KEY_Q, [0x11] = KEY_W, [0x12] = KEY_E, [0x13] = KEY_R,
    [0x14] = KEY_T, [0x15] = KEY_Y, [0x16] = KEY_U, [0x17] = KEY_I,
    [0x18] = KEY_O, [0x19] = KEY_P, [0x1A] = KEY_LBRACKET, [0x1B] = KEY_RBRACKET,
    [0x1C] = KEY_ENTER, [0x1D] = KEY_LCTRL, [0x1E] = KEY_A, [0x1F] = KEY_S,
    [0x20] = KEY_D, [0x21] = KEY_F, [0x22] = KEY_G, [0x23] = KEY_H,
    [0x24] = KEY_J, [0x25] = KEY_K, [0x26] = KEY_L, [0x27] = KEY_SEMICOLON,
    [0x28] = KEY_QUOTE, [0x29] = KEY_BACKTICK, [0x2A] = KEY_LSHIFT,
    [0x2B] = KEY_BACKSLASH, [0x2C] = KEY_Z, [0x2D] = KEY_X, [0x2E] = KEY_C,
    [0x2F] = KEY_V, [0x30] = KEY_B, [0x31] = KEY_N, [0x32] = KEY_M,
    [0x33] = KEY_COMMA, [0x34] = KEY_PERIOD, [0x35] = KEY_SLASH,
    [0x36] = KEY_RSHIFT, [0x37] = KEY_NUMPAD_MULTIPLY, [0x38] = KEY_LALT,
    [0x39] = KEY_SPACE, [0x3A] = KEY_CAPSLOCK, [0x3B] = KEY_F1, [0x3C] = KEY_F2,
    [0x3D] = KEY_F3, [0x3E] = KEY_F4, [0x3F] = KEY_F5, [0x40] = KEY_F6,
    [0x41] = KEY_F7, [0x42] = KEY_F8, [0x43] = KEY_F9, [0x44] = KEY_F10,
    [0x45] = KEY_NUMLOCK, [0x46] = KEY_SCROLLLOCK, [0x47] = KEY_NUMPAD7,
    [0x48] = KEY_NUMPAD8, [0x49] = KEY_NUMPAD9, [0x4A] = KEY_NUMPAD_MINUS,
    [0x4B] = KEY_NUMPAD4, [0x4C] = KEY_NUMPAD5, [0x4D] = KEY_NUMPAD6,
    [0x4E] = KEY_NUMPAD_PLUS, [0x4F] = KEY_NUMPAD1, [0x50] = KEY_NUMPAD2,
    [0x51] = KEY_NUMPAD3, [0x52] = KEY_NUMPAD0, [0x53] = KEY_NUMPAD_PERIOD,
    [0x57] = KEY_F11, [0x58] = KEY_F12
};

// Convert scancode to keycode, handling extended keys separately
static enum keycode scancode_to_keycode(uint8_t scancode) {
    if (extended) {
        extended = 0;
        switch (scancode) {
            case 0x1C: return KEY_NUMPAD_ENTER;
            case 0x35: return KEY_NUMPAD_DIVIDE;
            case 0x37: return KEY_PRINTSCREEN;
            case 0x45: return KEY_PAUSE;
            case 0x47: return KEY_HOME;
            case 0x48: return KEY_UP;
            case 0x49: return KEY_PAGEUP;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            case 0x4F: return KEY_END;
            case 0x50: return KEY_DOWN;
            case 0x51: return KEY_PAGEDOWN;
            case 0x52: return KEY_INSERT;
            case 0x53: return KEY_DELETE;
            case 0x20: return KEY_MUTE;
            case 0x2E: return KEY_VOLUMEDOWN;
            case 0x30: return KEY_VOLUMEUP;
            case 0x6D: return KEY_BRIGHTNESSDOWN;
            case 0x6F: return KEY_BRIGHTNESSUP;
            default: return KEY_UNKNOWN;
        }
    }
    if (scancode < 128) {
        return scancode_table[scancode];
    }
    return KEY_UNKNOWN;
}

// ASCII conversion for unshifted printable keys
static const char keycode_to_ascii_unshifted[] = {
    [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd', [KEY_E] = 'e',
    [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h', [KEY_I] = 'i', [KEY_J] = 'j',
    [KEY_K] = 'k', [KEY_L] = 'l', [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o',
    [KEY_P] = 'p', [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
    [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x', [KEY_Y] = 'y',
    [KEY_Z] = 'z',
    [KEY_0] = '0', [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_4] = '4',
    [KEY_5] = '5', [KEY_6] = '6', [KEY_7] = '7', [KEY_8] = '8', [KEY_9] = '9',
    [KEY_SPACE] = ' ', [KEY_ENTER] = '\n', [KEY_BACKSPACE] = '\b',
    [KEY_TAB] = '\t', [KEY_MINUS] = '-', [KEY_EQUAL] = '=',
    [KEY_LBRACKET] = '[', [KEY_RBRACKET] = ']', [KEY_SEMICOLON] = ';',
    [KEY_QUOTE] = '\'', [KEY_BACKTICK] = '`', [KEY_COMMA] = ',',
    [KEY_PERIOD] = '.', [KEY_SLASH] = '/', [KEY_BACKSLASH] = '\\',
    [KEY_NUMPAD0] = '0', [KEY_NUMPAD1] = '1', [KEY_NUMPAD2] = '2',
    [KEY_NUMPAD3] = '3', [KEY_NUMPAD4] = '4', [KEY_NUMPAD5] = '5',
    [KEY_NUMPAD6] = '6', [KEY_NUMPAD7] = '7', [KEY_NUMPAD8] = '8',
    [KEY_NUMPAD9] = '9', [KEY_NUMPAD_MULTIPLY] = '*', [KEY_NUMPAD_PLUS] = '+',
    [KEY_NUMPAD_MINUS] = '-', [KEY_NUMPAD_PERIOD] = '.', [KEY_NUMPAD_DIVIDE] = '/',
    [KEY_NUMPAD_ENTER] = '\n'
};

// Shifted ASCII table (where different)
static const char keycode_to_ascii_shifted[] = {
    [KEY_0] = ')', [KEY_1] = '!', [KEY_2] = '@', [KEY_3] = '#', [KEY_4] = '$',
    [KEY_5] = '%', [KEY_6] = '^', [KEY_7] = '&', [KEY_8] = '*', [KEY_9] = '(',
    [KEY_MINUS] = '_', [KEY_EQUAL] = '+', [KEY_LBRACKET] = '{',
    [KEY_RBRACKET] = '}', [KEY_SEMICOLON] = ':', [KEY_QUOTE] = '"',
    [KEY_BACKTICK] = '~', [KEY_COMMA] = '<', [KEY_PERIOD] = '>',
    [KEY_SLASH] = '?', [KEY_BACKSLASH] = '|'
};

// Add a key event to the circular buffer
static void queue_event(enum keycode code, enum key_event_type type) {
    uint32_t next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head == buffer_tail) return; // Buffer full, drop event
    buffer[buffer_head].code = code;
    buffer[buffer_head].type = type;
    buffer[buffer_head].modifiers = modifiers;
    buffer_head = next_head;
}

// Process incoming scancode from ISR
static void keyboard_handle_scancode(uint8_t scancode) {
    enum keycode code;

    if (scancode == 0xE0) {
        extended = 1; // Mark next scancode as extended
        return;
    }

    int released = scancode & 0x80; // Check if release event (bit 7 set)
    if (released) scancode &= 0x7F; // Clear release bit
    code = scancode_to_keycode(scancode);

    if (code == KEY_UNKNOWN) return; // Ignore unmapped scancodes

    // Handle modifier keys
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
        case KEY_CAPSLOCK:
            if (!released) modifiers ^= MOD_CAPSLOCK; // Toggle on press
            break;
        case KEY_NUMLOCK:
            if (!released) modifiers ^= MOD_NUMLOCK; // Toggle on press
            break;
        default:
            queue_event(code, released ? KEY_RELEASE : KEY_PRESS);
            break;
    }
}

// Initialize the keyboard driver
static int keyboard_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
    modifiers = 0;
    extended = 0;
    isr_keyboard_register_handler(keyboard_handle_scancode); // Register with ISR
    return 0;
}

// Get the next key event from the buffer
static int keyboard_get_event(struct key_event *event) {
    if (buffer_head == buffer_tail) return 0; // Buffer empty
    *event = buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE; // Move tail forward
    return 1;
}

// Return current modifier state
static uint8_t keyboard_get_modifiers(void) {
    return modifiers;
}

// Check if the keycode is a numpad number key
static int is_numpad_number(enum keycode code) {
    switch (code) {
        case KEY_NUMPAD0:
        case KEY_NUMPAD1:
        case KEY_NUMPAD2:
        case KEY_NUMPAD3:
        case KEY_NUMPAD4:
        case KEY_NUMPAD5:
        case KEY_NUMPAD6:
        case KEY_NUMPAD7:
        case KEY_NUMPAD8:
        case KEY_NUMPAD9:
            return 1;
        default:
            return 0;
    }
}

// Convert keycode to ASCII, considering modifiers
static char keyboard_keycode_to_ascii(enum keycode code, uint8_t modifiers) {
    char base = keycode_to_ascii_unshifted[code];
    if (!base) return 0; // Non-printable key

    // Separate Shift and Caps Lock states
    int shift_active = !!(modifiers & MOD_SHIFT);
    int caps_active = !!(modifiers & MOD_CAPSLOCK);
    
    // Numpad: Printable only if Num Lock is on for numpad number keys
    if (is_numpad_number(code) && !(modifiers & MOD_NUMLOCK)) {
        return 0; // Return nothing if Num Lock off (treated as navigation)
    }

    // Letters: Uppercase if exactly one of Shift or Caps Lock is active
    if ('a' <= base && base <= 'z') {
        if (shift_active ^ caps_active) { // XOR: true if only one is active
            return base - 32; // To uppercase
        }
        return base; // Remains lowercase or stays as is
    }

    // Symbols: Shift active produces shifted version, Caps Lock ignored
    if (shift_active && keycode_to_ascii_shifted[code]) {
        return keycode_to_ascii_shifted[code];
    }

    return base; // Default unshifted character
}

// Clear the keyboard event buffer
static void keyboard_clear_buffer(void) {
    buffer_head = 0; // Reset head to start
    buffer_tail = 0; // Reset tail to start, making buffer empty
}

// Driver instance
struct keyboard_driver keyboard_drv = {
    .init = keyboard_init,
    .get_event = keyboard_get_event,
    .get_modifiers = keyboard_get_modifiers,
    .keycode_to_ascii = keyboard_keycode_to_ascii,
    .clear_buffer = keyboard_clear_buffer
};