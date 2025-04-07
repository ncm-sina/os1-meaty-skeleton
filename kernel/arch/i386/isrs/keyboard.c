#include <kernel/isrs/keyboard.h>
#include <kernel/mport.h>

uint8_t isr_keyboard_latest_scancode = 0;
volatile int isr_keyboard_data_available = 0;
static void (*keyboard_handler)(uint8_t scancode) = NULL;

void isr_keyboard(void) {
    isr_keyboard_latest_scancode = inb(0x60);
    isr_keyboard_data_available = 1;
    if (keyboard_handler) {
        keyboard_handler(isr_keyboard_latest_scancode);
    }
}

void isr_keyboard_register_handler(void (*handler)(uint8_t scancode)) {
    keyboard_handler = handler;
}