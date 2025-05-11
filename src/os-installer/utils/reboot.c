#include "reboot.h"
#include <stdint.h>
#include <kernel/mport.h>

/* Reboot via keyboard controller (default) */
void reboot_kbd(void) {
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 0x01) inb(0x60); /* Clear buffer */
    } while (temp & 0x02); /* Wait for input buffer empty */
    outb(0x64, 0xFE); /* Pulse reset line */
    for(;;) asm("hlt");
}

/* Reboot via ACPI (placeholder, requires ACPI tables) */
void reboot_acpi(void) {
    /* TODO: Implement ACPI reset */
    reboot_kbd();
}

/* Reboot via triple fault */
void reboot_triple_fault(void) {
    // asm volatile("lidt $0");
    // asm volatile("int 0x10");
    for(;;) asm("hlt");
}

/* Default reboot method */
void reboot(void) {
    reboot_kbd();
}
