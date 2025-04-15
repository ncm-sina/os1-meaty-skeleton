#ifndef DRIVERS_TIMER_H
#define DRIVERS_TIMER_H

#include <stdint.h>

struct timer_driver {
    void (*init)(void);
    uint32_t (*get_ticks)(void);
};

extern struct timer_driver timer_drv;

#endif