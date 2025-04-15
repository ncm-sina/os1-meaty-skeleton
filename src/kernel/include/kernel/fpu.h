#ifndef KERNEL_FPU_H
#define KERNEL_FPU_H

#include <stdint.h>

// Enable the FPU
void enable_fpu(void);

// Disable the FPU (for testing or power-saving, not typically needed)
void disable_fpu(void);

// Check if the FPU is enabled
int is_fpu_enabled(void);

#endif