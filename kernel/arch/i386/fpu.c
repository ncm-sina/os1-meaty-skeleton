#include <kernel/fpu.h>

void enable_fpu(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear EM (emulation) bit: no software emulation
    cr0 |= (1 << 1);  // Set MP (monitor coprocessor) bit: enable FPU exceptions
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    asm volatile("finit"); // Initialize FPU to default state
}

void disable_fpu(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 2);  // Set EM bit: disable FPU, emulate in software (triggers ISR 7)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

int is_fpu_enabled(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return !(cr0 & (1 << 2)); // Return 1 if EM bit is clear (FPU enabled)
}