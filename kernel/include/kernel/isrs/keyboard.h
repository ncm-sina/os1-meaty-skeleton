#ifndef ISRS_KEYBOARD_H
#define ISRS_KEYBOARD_H

#include <stdint.h>

extern uint8_t isr_keyboard_latest_scancode;
extern volatile int isr_keyboard_data_available;

void isr_keyboard(void);
void isr_keyboard_register_handler(void (*handler)(uint8_t scancode));

#endif