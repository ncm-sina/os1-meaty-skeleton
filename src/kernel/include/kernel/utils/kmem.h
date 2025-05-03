// TODO: Implement kmem
#ifndef KERNEL_FPU_H
#define KERNEL_FPU_H

#include <stdint.h>

void* kmalloc(size_t size);
void kfree(void* ptr);
void heap_init(void);

#endif