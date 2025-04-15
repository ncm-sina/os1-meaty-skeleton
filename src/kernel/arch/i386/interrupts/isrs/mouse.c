#include <kernel/isrs/mouse.h>
#include <kernel/mport.h>

static void (*mouse_handler)(uint8_t data) = NULL;

// Wait for PS/2 controller to be ready to read
static void mouse_wait_read(void) {
    while (!(inb(0x64) & 0x01)); // Wait for output buffer full
}

// Wait for PS/2 controller to be ready to write
static void mouse_wait_write(void) {
    while (inb(0x64) & 0x02); // Wait for input buffer empty
}

// Send a command to the mouse
static void mouse_write(uint8_t value) {
    mouse_wait_write();
    outb(0x64, 0xD4); // Tell controller we're sending to mouse
    mouse_wait_write();
    outb(0x60, value); // Send the command
}

// Read a byte from the mouse
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(0x60);
}

// Mouse ISR: Reads data byte and calls handler
void isr_mouse(void) {
    uint8_t data = inb(0x60); // Read from data port
    if (mouse_handler) {
        mouse_handler(data);
    }
    outb(0xA0, 0x20); // EOI to slave PIC (IRQ12)
    outb(0x20, 0x20); // EOI to master PIC
}

// Register a handler for mouse data
void isr_mouse_register_handler(void (*handler)(uint8_t data)) {
    mouse_handler = handler;
}