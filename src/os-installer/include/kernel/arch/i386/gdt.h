#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

#include <stdint.h>

// Number of GDT entries
#define GDT_ENTRIES 24

// GDT entry structure (packed to match x86 requirements)
typedef struct {
    uint16_t limit_low;    // Lower 16 bits of segment limit
    uint16_t base_low;     // Lower 16 bits of segment base
    uint8_t base_middle;   // Middle 8 bits of segment base
    uint8_t access;        // Access byte (privilege, type, etc.)
    uint8_t granularity;   // Granularity and upper 4 bits of limit
    uint8_t base_high;     // Upper 8 bits of segment base
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure for lgdt instruction
typedef struct {
    uint16_t limit;        // Size of GDT - 1 (in bytes)
    uint32_t base;         // Base address of GDT
} __attribute__((packed)) gdt_descriptor_t;

#define KCODE32_SEG equ 0x08     ; 32-bit code segment
#define KDATA32_SEG equ 0x10     ; 32-bit data segment
#define UCODE32_SEG equ 0x18     ; 16-bit code segment
#define UDATA32_SEG equ 0x20     ; 16-bit data segment
#define CODE16_SEG equ 0x28     ; 16-bit code segment
#define DATA16_SEG equ 0x30     ; 16-bit data segment

// Initialize the GDT with ring 0 and ring 3 segments
// void init_gdt(void);
static inline gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void init_gdt(void);


#endif