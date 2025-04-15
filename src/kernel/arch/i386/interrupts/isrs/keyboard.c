#include <kernel/isrs/keyboard.h>
#include <kernel/mport.h>

// Global variables accessible to other files via header
uint8_t isr_keyboard_latest_scancode = 0;
volatile int isr_keyboard_data_available = 0;
static void (*keyboard_handler)(uint8_t scancode) = NULL; // Handler remains static, only accessed via register function

// Keyboard ISR: Reads scancode from port 0x60 and calls registered handler
void isr_keyboard(void) {
    isr_keyboard_latest_scancode = inb(0x60);
    isr_keyboard_data_available = 1;
    if (keyboard_handler) {
        keyboard_handler(isr_keyboard_latest_scancode);
    }
}

// Register a handler function to process keyboard scancodes
void isr_keyboard_register_handler(void (*handler)(uint8_t scancode)) {
    keyboard_handler = handler;
}