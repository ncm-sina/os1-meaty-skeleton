#include <kernel/idt.h>
#include <kernel/mport.h>
#include <kernel/isrs/all.h>

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_descriptor idt_desc = {
    .size = sizeof(idt) - 1,
    .offset = (uint32_t)&idt /*+ 0xC0000000*/  /* Virtual address */
};

extern void idt_load(uint32_t idt_desc_addr);

extern void isr_default(void);
extern void isr0(void), isr1(void), isr14(void), isr32(void), isr33(void);
// extern void *isr_default, *isr0, *isr14, *isr32;

void *isr_stubs[IDT_ENTRIES];

static void pic_remap(void) {
    uint8_t pic1_mask = inb(0x21);
    uint8_t pic2_mask = inb(0xA1);

    outb(0x20, 0x11); outb(0xA0, 0x11); // ICW1: Initialize
    outb(0x21, 0x20); outb(0xA1, 0x28); // ICW2: Remap to 32, 40
    outb(0x21, 0x04); outb(0xA1, 0x02); // ICW3: Master/Slave
    outb(0x21, 0x01); outb(0xA1, 0x01); // ICW4: 8086 mode

    outb(0x21, pic1_mask);
    outb(0xA1, pic2_mask);
}

void isr_handler(uint32_t vector, uint32_t error_code) {
    if (vector < 32) {
        switch(vector){
            case 0:
                // not implemented yet
                break;
            case 14:
                // not implemented yet
                break;
            // default:
        }
        if (vector == 0) { /* Divide error: Panic */ }
        else if (vector == 14) { /* Page fault: Check CR2 */ }
    } else if (vector >= 32 && vector <= 47) {
        switch(vector){
            case 32:
                isr_timer();
                break;
            case 33:
                isr_keyboard();        
                break;
            // default:            
        }
            
        if (vector >= 40)
            outb(0xA0, 0x20);
        // else
            outb(0x20, 0x20);
    }else{
        // not implemented yet
    }
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

void idt_init(void) {
    
    // for (int i = 0; i < IDT_ENTRIES; i++)   isr_stubs[i] = isr_default;
    
    isr_stubs[0] = isr0;
    isr_stubs[1] = isr1;
    isr_stubs[14] = isr14;
    isr_stubs[32] = isr32;
    isr_stubs[33] = isr33;
    
    for (int i = 0; i < IDT_ENTRIES; i++) {
        if (!isr_stubs[i]) isr_stubs[i] = isr_default;
        idt_set_gate(i, (uint32_t)isr_stubs[i], 0x08, 0x8E);
    }
    
    pic_remap();
    // outb(0x21, inb(0x21) & ~(1 << 0 | 1 << 1));
    idt_load((uint32_t)&idt_desc);
    asm volatile("sti");
}