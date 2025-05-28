#include <kernel/arch/i386/idt.h>
#include <kernel/mport.h>
#include <kernel/arch/i386/isrs/all.h>
#include <kernel/drivers/all.h>
#include <kernel/arch/i386/syscall.h>
#include <kernel/drivers/ide.h>

#include <stdio.h>

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_descriptor idt_desc = {
    .size = sizeof(idt) - 1,
    .offset = (uint32_t)&idt 
};

extern void idt_load(uint32_t idt_desc_addr);
extern void isr_default(void);
extern void isr0(void), isr1(void), isr2(void), isr3(void), isr4(void), isr5(void),
            isr6(void), isr7(void), isr8(void), isr10(void), isr11(void), isr12(void),
            isr13(void), isr14(void), isr32(void), isr33(void), isr44(void), isr46(void), isr128(void); // Added isr44

void *isr_stubs[IDT_ENTRIES];

static void pic_remap(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11); // ICW1: Initialize PICs
    outb(0x21, 0x20); outb(0xA1, 0x28); // ICW2: Base vectors (32, 40)
    outb(0x21, 0x04); outb(0xA1, 0x02); // ICW3: Master/Slave wiring
    outb(0x21, 0x01); outb(0xA1, 0x01); // ICW4: 8086 mode
    outb(0x21, 0xF8); // Unmask IRQ0 (timer), IRQ1 (keyboard), IRQ2 (cascade)
    outb(0xA1, 0xEF); // Unmask IRQ12 (mouse), mask others
}

void isr_handler(uint32_t vector, uint32_t error_code) {
    if (vector < 32) {
        switch (vector) {
            case 0:  isr_divide_error(); break;
            case 1:  isr_debug(); break;
            case 2:  isr_nmi(); break;
            case 3:  isr_breakpoint(); break;
            case 4:  isr_overflow(); break;
            case 5:  isr_bound_range(); break;
            case 6:  isr_invalid_opcode(); break;
            case 7:  isr_device_not_available(); break;
            case 8:  isr_double_fault(error_code); break;
            case 10: isr_invalid_tss(error_code); break;
            case 11: isr_segment_not_present(error_code); break;
            case 12: isr_stack_segment_fault(error_code); break;
            case 13: isr_general_protection(error_code); break;
            case 14: isr_page_fault(error_code); break;
            default: printf("Unhandled exception: %d\n", vector); break;
        }
    } else if (vector >= 32 && vector <= 47) {
        switch (vector) {
            case 32: isr_timer(); break;    // IRQ0: Timer
            case 33: isr_keyboard(); break; // IRQ1: Keyboard
            case 44: isr_mouse(); break;    // IRQ12: Mouse
            case 46: ide_irq_handler(); break;
            default: printf("Unhandled IRQ: %d\n", vector); break;
        }
        if (vector >= 40)
            outb(0xA0, 0x20); // EOI to slave PIC
        outb(0x20, 0x20);     // EOI to master PIC
    } else {
        printf("Unknown vector: %d\n", vector);
    }
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

int idt_init(void) {
    isr_stubs[0] = isr0;
    isr_stubs[1] = isr1;
    isr_stubs[2] = isr2;
    isr_stubs[3] = isr3;
    isr_stubs[4] = isr4;
    isr_stubs[5] = isr5;
    isr_stubs[6] = isr6;
    isr_stubs[7] = isr7;
    isr_stubs[8] = isr8;
    isr_stubs[10] = isr10;
    isr_stubs[11] = isr11;
    isr_stubs[12] = isr12;
    isr_stubs[13] = isr13;
    isr_stubs[14] = isr14;
    isr_stubs[32] = isr32; // Timer ISR
    isr_stubs[33] = isr33; // Keyboard ISR
    isr_stubs[44] = isr44; // Mouse ISR

    isr_stubs[46] = isr46; // Mouse ISR

    isr_stubs[128] = isr128;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        if (!isr_stubs[i]) isr_stubs[i] = isr_default;
        idt_set_gate(i, (uint32_t)isr_stubs[i], 0x08, 0x8E);
    }

    idt_set_gate(128, (unsigned)syscall_handler, 0x08, 0x8E | 0x60); // DPL=3 for user access

    pic_remap();
    idt_load((uint32_t)&idt_desc);
    asm volatile("sti"); // Enable interrupts
    return 0;
}