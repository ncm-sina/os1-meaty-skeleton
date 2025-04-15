#ifndef ISRS_TIMER_H
#define ISRS_TIMER_H

#include <stdint.h>

extern volatile int isr_timer_tick_flag; // Flag indicating a timer tick

// ISR entry point for timer interrupt (IRQ0)
extern void isr_timer(void);
// Function to register a handler for timer ticks
extern void isr_timer_register_handler(void (*handler)(void));

#endif