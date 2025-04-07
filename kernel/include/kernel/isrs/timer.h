#ifndef ISRS_TIMER_H
#define ISRS_TIMER_H

#include <stdint.h>

extern volatile int isr_timer_tick_flag;

void isr_timer(void);
void isr_timer_register_handler(void (*handler)(void));

#endif