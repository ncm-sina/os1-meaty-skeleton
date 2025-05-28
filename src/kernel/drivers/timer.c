#include <kernel/drivers/timer.h>
#include <kernel/arch/i386/isrs/timer.h>
#include <kernel/mport.h>

uint32_t timer_ticks = 0;
uint32_t i=0;

static void timer_handle_tick(void) {
    timer_ticks++;
    // if(timer_ticks % 10 == 0){
    //     serial_printf(" %d ", timer_ticks);
    // }
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

static int timer_init(void) {

    set_tick_hz();

    timer_ticks = 0;
    isr_timer_register_handler(timer_handle_tick);
    return 0;
}

static uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

struct timer_driver timer_drv = {
    .init = timer_init,
    .get_ticks = timer_get_ticks
};