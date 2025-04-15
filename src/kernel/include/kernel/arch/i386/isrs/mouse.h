#ifndef ISRS_MOUSE_H
#define ISRS_MOUSE_H

#include <stdint.h>

// ISR entry point for mouse interrupt (IRQ12)
extern void isr_mouse(void);
// Register a handler for mouse data (called with each packet byte)
extern void isr_mouse_register_handler(void (*handler)(uint8_t data));

#endif