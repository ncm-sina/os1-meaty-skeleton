#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/isrs/timer.h>

volatile int isr_timer_tick_flag = 0;
static void (*timer_handler)(void) = NULL;

void isr_timer(void) {
    isr_timer_tick_flag = 1;
    if (timer_handler) {
        timer_handler();
    }
}

void isr_timer_register_handler(void (*handler)(void)) {
    timer_handler = handler;
}