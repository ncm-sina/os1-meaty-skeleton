#include <a20.h>
#include <libk/io.h>

// Keyboard controller ports
#define KBD_STATUS_PORT 0x64
#define KBD_COMMAND_PORT 0x64
#define KBD_DATA_PORT 0x60

// Keyboard controller commands
#define KBD_READ_OUTPUT 0xD0
#define KBD_WRITE_OUTPUT 0xD1
#define KBD_ENABLE_A20 0xDF

// Wait for keyboard controller to be ready
static void kbd_wait_input(void) {
    while (inb(KBD_STATUS_PORT) & 0x02);
}

static void kbd_wait_output(void) {
    while (!(inb(KBD_STATUS_PORT) & 0x01));
}

void a20_enable(void) {
    // Read output port
    kbd_wait_input();
    outb(KBD_COMMAND_PORT, KBD_READ_OUTPUT);
    kbd_wait_output();
    uint8_t output = inb(KBD_DATA_PORT);

    // Enable A20 bit (bit 1)
    output |= 0x02;

    // Write back to output port
    kbd_wait_input();
    outb(KBD_COMMAND_PORT, KBD_WRITE_OUTPUT);
    kbd_wait_input();
    outb(KBD_DATA_PORT, output);
}