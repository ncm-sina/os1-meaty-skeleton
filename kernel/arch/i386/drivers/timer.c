#include <kernel/drivers/timer.h>
#include <kernel/isrs/timer.h>

static uint32_t ticks = 0;

static void timer_handle_tick(void) {
    ticks++;
}

static void timer_init(void) {
    ticks = 0;
    isr_timer_register_handler(timer_handle_tick);
}

static uint32_t timer_get_ticks(void) {
    return ticks;
}

struct timer_driver timer_drv = {
    .init = timer_init,
    .get_ticks = timer_get_ticks
};