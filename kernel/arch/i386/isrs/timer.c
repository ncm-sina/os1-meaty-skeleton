#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/isrs/timer.h>

volatile int isr_timer_tick_flag = 0;
static void (*timer_handler)(void) = NULL; // Handler remains static, only accessed via register function

// Timer ISR: Sets tick flag and calls registered handler
void isr_timer(void) {
    isr_timer_tick_flag = 1;
    if (timer_handler) {
        timer_handler();
    }
}

// Register a handler function to process timer ticks
void isr_timer_register_handler(void (*handler)(void)) {
    timer_handler = handler;
}