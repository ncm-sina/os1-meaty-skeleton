#include <kernel/arch/i386/gdt.h>

#define GDT_PHYS_ADDR 0x90000
#define GDT_DESC_PHYS_ADDR 0x900F0
#define GDT_NUM_ENTRIES 7


// Define GDT at fixed location
gdt_entry_t* gdt = (gdt_entry_t*) GDT_PHYS_ADDR;
gdt_descriptor_t* gdt_desc = (gdt_descriptor_t*) GDT_DESC_PHYS_ADDR;


// Set a single GDT entry
static inline gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;          // Lower 16 bits of base
    gdt[num].base_middle = (base >> 16) & 0xFF; // Middle 8 bits of base
    gdt[num].base_high = (base >> 24) & 0xFF;   // Upper 8 bits of base
    gdt[num].limit_low = limit & 0xFFFF;        // Lower 16 bits of limit
    gdt[num].granularity = (limit >> 16) & 0x0F;// Upper 4 bits of limit
    gdt[num].granularity |= gran & 0xF0;        // Granularity flags (e.g., 4KB pages)
    gdt[num].access = access;                   // Access byte
}

// Initialize the GDT with kernel and user segments
void init_gdt(void) {
    // Null descriptor (entry 0)
    gdt_set_gate(0, 0, 0, 0, 0);

    // Kernel code segment (ring 0): selector 0x08
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // Access: 0x9A = Present (1) | Ring 0 (00) | Code/Data (1) | Executable (1) | Readable (0)
    // Granularity: 0xCF = 4KB pages (1) | 32-bit (1) | Limit 4GB

    // Kernel data segment (ring 0): selector 0x10
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    // Access: 0x92 = Present (1) | Ring 0 (00) | Code/Data (1) | Executable (0) | Writable (1)
    // Granularity: 0xCF = 4KB pages (1) | 32-bit (1) | Limit 4GB

    // User code segment (ring 3): selector 0x18 | RPL 3 = 0x1B
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    // Access: 0xFA = Present (1) | Ring 3 (11) | Code/Data (1) | Executable (1) | Readable (0)
    // Granularity: 0xCF = 4KB pages (1) | 32-bit (1) | Limit 4GB

    // User data segment (ring 3): selector 0x20 | RPL 3 = 0x23
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    // Access: 0xF2 = Present (1) | Ring 3 (11) | Code/Data (1) | Executable (0) | Writable (1)
    // Granularity: 0xCF = 4KB pages (1) | 32-bit (1) | Limit 4GB

    // 16-bit code segment (ring 0): selector 0x28
    gdt_set_gate(5, 0, 0xFFFF, 0x9A, 0x0F);
    // Access: 0x9A = Present (1) | Ring 0 (00) | Code/Data (1) | Executable (1) | Readable (0)
    // Granularity: 0x0F = Byte granularity (0) | 16-bit (0) | Limit 64KB

    // 16-bit data segment (ring 0): selector 0x30
    gdt_set_gate(6, 0, 0xFFFF, 0x92, 0x0F);
    // Access: 0x92 = Present (1) | Ring 0 (00) | Code/Data (1) | Executable (0) | Writable (1)
    // Granularity: 0x0F = Byte granularity (0) | 16-bit (0) | Limit 64KB
    
    // Load the GDT
    gdt_desc->limit = (GDT_NUM_ENTRIES * sizeof(gdt_entry_t)) - 1; // Size of GDT in bytes - 1
    gdt_desc->base = (uint32_t)GDT_PHYS_ADDR;   // Physical address of GDT
    asm volatile("lgdt %0" : : "m" (*gdt_desc));

    // Reload segment registers for kernel mode
    asm volatile(
        "mov $0x10, %ax\n"   // Kernel data segment
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "ljmp $0x08, $1f\n"  // Kernel code segment
        "1:"
    );
}