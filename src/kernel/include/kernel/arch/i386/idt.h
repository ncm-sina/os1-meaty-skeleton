#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;   // Lower 16 bits of handler address
    uint16_t selector;     // GDT segment selector
    uint8_t  zero;         // Always 0
    uint8_t  type_attr;    // Gate type and attributes
    uint16_t offset_high;  // Upper 16 bits of handler address
} __attribute__((packed));

struct idt_descriptor {
    uint16_t size;         // Size of IDT - 1
    uint32_t offset;       // Base address of IDT
} __attribute__((packed));

extern struct idt_entry idt[256];  // Expose idt array
extern struct idt_descriptor idt_desc; // Declare as extern for global access


void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void isr_handler(uint32_t vector, uint32_t error_code);
#endif