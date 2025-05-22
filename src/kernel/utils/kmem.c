#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/utils/kmem.h>
#include <stdio.h> // For printf debugging

#define HEAP_SIZE  0x2000000 // 8MB heap, as per your current setup
#define MIN_BLOCK_SIZE 16   // Minimum block size to avoid tiny fragments
#define ALIGNMENT 16        // 16-byte alignment for allocations

// Memory block header
typedef struct heap_block_t {
    uint32_t size;          // Size of the block (excluding header)
    uint32_t free;           // 1 if free, 0 if used
    struct heap_block_t* next; // Pointer to next block
    struct heap_block_t* prev; // Pointer to previous block
} __attribute__((packed)) heap_block_t;

// Static heap 16 byte aligned
__attribute__((aligned(16))) static uint8_t heap[HEAP_SIZE];
static heap_block_t *heap_start; // Head of the block list

// Align a size to the specified alignment
static inline uint32_t align_size(uint32_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}


// Initialize the heap
void init_heap(uint32_t heap_start_addr) {
    // Ignore heap_start since we're using a static array
    (void)heap_start_addr;
    
    // Initialize the first block to span the entire heap
    heap_block_t *heap_start = (heap_block_t *)heap;
    heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    
    printf("Heap initialized: start=%08x, size=0x%08x\n", heap_start, HEAP_SIZE);
}

// Allocate memory
void* kmalloc(uint32_t size) {
    if (size == 0) return NULL;
    
    // Align the requested size
    size = align_size(size);
    
    // Find a free block large enough
    heap_block_t* current = (heap_block_t *) heap;
    while (current && current>=heap && current<=(heap + HEAP_SIZE) ) {
        if (current->free && current->size >= size) {
            // Found a suitable block
            if (current->size >= size + sizeof(heap_block_t) + MIN_BLOCK_SIZE) {
                // Split the block
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->free = 1;
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
                current->size = size;
            }
            
            // Mark block as used
            current->free = 0;
            void *ptr = (uint8_t *)current + sizeof(heap_block_t);
            // printf("kmalloc: size=%u, ptr=%08x\n", size, ptr);
            return ptr;
        }
        current = current->next;
    }
    
    printf("kmalloc: out of memory for size=%08x\n heap_base:%08x heap_current:%08x ", size, heap, current);
    while(1);
    return NULL;
}

// Free memory
void kfree(void* ptr) {
    if (!ptr) return;
    
    // Find the block corresponding to ptr
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if ((uint8_t *)block < heap || (uint8_t *)block >= heap + HEAP_SIZE) {
        // printf("kfree: invalid pointer %08x\n , block=%08x, heap=%08x, heap_end=%08x ", ptr, block, heap, heap + HEAP_SIZE);
        return;
    }
    if (block->free) {
        printf("kfree: double free at %08x\n", ptr);
        return;
    }
    
    // printf("kfree: ptr=%08x, size=%u\n", ptr, block->size);
    block->free = 1;
    
    // Merge with next block if free
    if (block->next && block->next->free) {
        block->size += block->next->size + sizeof(heap_block_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Merge with previous block if free
    if (block->prev && block->prev->free) {
        block->prev->size += block->size + sizeof(heap_block_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}


// // Allocate memory
// void* kmalloc(uint32_t size) {
//     if (size == 0) return NULL;
    
//     // Align the requested size
//     size = align_size(size);
    
//     // Find a free block large enough
//     heap_block_t* current = (heap_block_t *) heap;
//     while (current) {
//         if (current->free && current->size >= size) {
//             // Found a suitable block
//             if (current->size >= size + sizeof(heap_block_t) + MIN_BLOCK_SIZE) {
//                 // Split the block
//                 heap_block_t *new_block = (heap_block_t *)((uint8_t *)current + sizeof(heap_block_t) + size);
//                 new_block->size = current->size - size - sizeof(heap_block_t);
//                 new_block->free = 1;
//                 new_block->next = current->next;
//                 new_block->prev = current;
                
//                 if (current->next) {
//                     current->next->prev = new_block;
//                 }
//                 current->next = new_block;
//                 current->size = size;
//             }
            
//             // Mark block as used
//             current->free = 0;
//             void *ptr = (uint8_t *)current + sizeof(heap_block_t);
//             // printf("kmalloc: size=%u, ptr=%08x\n", size, ptr);
//             return ptr;
//         }
//         current = current->next;
//     }
    
//     printf("kmalloc: out of memory for size=%08x\n heap_base:%08x heap_current:%08x ", size, heap, current);
//     while(1);
//     return NULL;
// }

// // Free memory
// void kfree(void* ptr) {
//     if (!ptr) return;
    
//     // Find the block corresponding to ptr
//     heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
//     if ((uint8_t *)block < heap || (uint8_t *)block >= heap + HEAP_SIZE) {
//         // printf("kfree: invalid pointer %08x\n , block=%08x, heap=%08x, heap_end=%08x ", ptr, block, heap, heap + HEAP_SIZE);
//         return;
//     }
//     if (block->free) {
//         printf("kfree: double free at %08x\n", ptr);
//         return;
//     }
    
//     // printf("kfree: ptr=%08x, size=%u\n", ptr, block->size);
//     block->free = 1;
    
//     // Merge with next block if free
//     if (block->next && block->next->free) {
//         block->size += block->next->size + sizeof(heap_block_t);
//         block->next = block->next->next;
//         if (block->next) {
//             block->next->prev = block;
//         }
//     }
    
//     // Merge with previous block if free
//     if (block->prev && block->prev->free) {
//         block->prev->size += block->size + sizeof(heap_block_t);
//         block->prev->next = block->next;
//         if (block->next) {
//             block->next->prev = block->prev;
//         }
//         block = block->prev;
//     }
// }

// #include "memory.h"

// // #define HEAP_START 0x1000000
// #define HEAP_SIZE  0x800000

// static uint8_t heap[HEAP_SIZE];
// uint32_t heap_pos = 0;

// void init_heap(uint32_t heap_start){
//     // we can use heap_start as our heap's start address
// }

// void* kmalloc(uint32_t size) {
//     if (heap_pos + size > HEAP_SIZE){
//         printf("heap: out of memory! ");
//         return NULL;
//     }
//     void* ptr = &heap[heap_pos];
//     // printf("hptr:%08x ", ptr);
//     heap_pos += (size + 0xf) & ~0xf; // Align to 16 bytes
//     return ptr;
// }

// void kfree(void* ptr) {
//     // Simplified: No deallocation
//     (void)ptr;
// }
