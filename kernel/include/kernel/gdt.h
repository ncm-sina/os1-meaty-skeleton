#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

#include <stdint.h>

// Number of GDT entries
#define GDT_ENTRIES 5

// GDT entry structure (packed to match x86 requirements)
typedef struct {
    uint16_t limit_low;    // Lower 16 bits of segment limit
    uint16_t base_low;     // Lower 16 bits of segment base
    uint8_t base_middle;   // Middle 8 bits of segment base
    uint8_t access;        // Access byte (privilege, type, etc.)
    uint8_t granularity;   // Granularity and upper 4 bits of limit
    uint8_t base_high;     // Upper 8 bits of segment base
} __attribute__((packed)) GdtEntry;

// GDT pointer structure for lgdt instruction
typedef struct {
    uint16_t limit;        // Size of GDT - 1 (in bytes)
    uint32_t base;         // Base address of GDT
} __attribute__((packed)) GdtPtr;

// Initialize the GDT with ring 0 and ring 3 segments
void init_gdt(void);

#endif