#include <kernel/drivers/timer.h>
#include <kernel/arch/i386/isrs/timer.h>
#include <kernel/mport.h>

static uint32_t ticks = 0;

static void timer_handle_tick(void) {
    ticks++;
}

static void set_tick_hz() { // here we change tick speed from 18.2Hz to 100Hz
    uint32_t divisor = 1193180 / 100; // 10ms ticks, ~11931
    uint8_t mode = 0x36;              // Mode 3 (square wave), channel 0
    uint8_t low_byte = divisor & 0xFF;
    uint8_t high_byte = (divisor >> 8) & 0xFF;

    outb(mode, 0x43);       // Set PIT mode
    outb(low_byte, 0x40);   // Send low byte to channel 0
    outb(high_byte, 0x40);  // Send high byte to channel 0
}

static void timer_init(void) {

    set_tick_hz();

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