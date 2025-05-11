#ifndef ISRS_KEYBOARD_H
#define ISRS_KEYBOARD_H

#include <stdint.h>

extern uint8_t isr_keyboard_latest_scancode;     // Latest scancode from keyboard interrupt
extern volatile int isr_keyboard_data_available; // Flag indicating new data

// ISR entry point for keyboard interrupt (IRQ1)
extern void isr_keyboard(void);
// Function to register a handler for keyboard scancodes
extern void isr_keyboard_register_handler(void (*handler)(uint8_t scancode));

#endif