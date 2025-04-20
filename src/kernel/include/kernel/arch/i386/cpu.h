#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #include <sys/types.h>

// CPU feature flags
struct cpu_features {
    uint32_t fpu : 1;    // Floating Point Unit
    uint32_t sse : 1;    // Streaming SIMD Extensions
    uint32_t sse2 : 1;   // SSE2
    uint32_t tsc : 1;    // Time Stamp Counter
    uint32_t apic : 1;   // Advanced Programmable Interrupt Controller
    uint32_t pae : 1;    // Physical Address Extension
};

// Initialize CPU
void cpu_init(void);

// Get CPU features
const struct cpu_features *cpu_get_features(void);

// CPU control
void cpu_halt(void);
void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);

// I/O port access

// Read timestamp counter
uint64_t read_tsc(void);

#endif