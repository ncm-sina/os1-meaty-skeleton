#ifndef KMEM_H
#define KMEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void init_heap(uint32_t heap_start_addr);
void* kmalloc(uint32_t size);
void kfree(void* ptr);

#endif // MEMORY_H