#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/utils/kmem.h>

// External page allocator (assumed to be implemented elsewhere)
extern uint32_t alloc_page(void);
extern void free_page(uint32_t paddr);

// Heap configuration
#define HEAP_START 0xC0100000
#define HEAP_INITIAL_SIZE (4 * 4096) // 4 pages initially
#define MIN_BLOCK_SIZE 16 // Minimum block size to prevent excessive fragmentation
#define ALIGNMENT 16        // 16-byte alignment for allocations


// Block header structure
typedef struct Block {
    size_t size;         // Size of the block (including header)
    int free;            // 1 if free, 0 if allocated
    struct Block* next;   // Pointer to the next block
} Block;

// Global free list
static Block* free_list = NULL;
static uint32_t heap_end = HEAP_START;

// Align size to 8 bytes
static inline size_t align8(size_t size) {
    return (size + 7) & ~7;
}

static inline size_t align16(size_t size) {
    return (size + 0xF) & ~0xF;
}

// Initialize the heap
void heap_init(void) {
    // Allocate initial heap pages
    uint32_t paddr = alloc_page();
    if (!paddr) {
        // Panic: no memory
        return;
    }

    // Map the physical page to HEAP_START (assumed paging is set up)
    // In a real kernel, you'd update the page tables here
    Block* initial_block = (Block*)HEAP_START;
    initial_block->size = HEAP_INITIAL_SIZE;
    initial_block->free = 1;
    initial_block->next = NULL;

    free_list = initial_block;
    heap_end = HEAP_START + HEAP_INITIAL_SIZE;
}

// Extend the heap by allocating a new page
static Block* extend_heap(size_t size) {
    uint32_t paddr = alloc_page();
    if (!paddr) {
        return NULL;
    }

    // Map the new page at heap_end (update page tables in a real kernel)
    Block* new_block = (Block*)heap_end;
    new_block->size = 4096; // One page
    new_block->free = 1;
    new_block->next = NULL;

    heap_end += 4096;
    return new_block;
}

// Find a free block using first-fit
static Block* find_free_block(size_t size) {
    Block* current = free_list;
    while (current && !(current->free && current->size >= size)) {
        current = current->next;
    }
    return current;
}

// Split a block if it's too large
static void split_block(Block* block, size_t size) {
    if (block->size >= size + sizeof(Block) + MIN_BLOCK_SIZE) {
        Block* new_block = (Block*)((char*)block + sizeof(Block) + size);
        new_block->size = block->size - size - sizeof(Block);
        new_block->free = 1;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Align size and add header
    size = align8(size);
    size_t total_size = size + sizeof(Block);

    // Initialize heap if not done
    if (!free_list) {
        heap_init();
    }

    // Find a free block
    Block* block = find_free_block(total_size);
    if (!block) {
        // Extend heap if no suitable block is found
        block = extend_heap(total_size);
        if (!block) {
            return NULL;
        }
        // Add new block to free list
        block->next = free_list;
        free_list = block;
    }

    // Mark block as allocated
    block->free = 0;
    split_block(block, size);

    // Return pointer to the data (after header)
    return (char*)block + sizeof(Block);
}

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    // Get the block header
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->free = 1;

    // Merge with next block if free
    if (block->next && block->next->free) {
        block->size += block->next->size + sizeof(Block);
        block->next = block->next->next;
    }

    // Merge with previous block if free
    Block* current = free_list;
    Block* prev = NULL;
    while (current && current != block) {
        prev = current;
        current = current->next;
    }
    if (prev && prev->free) {
        prev->size += block->size + sizeof(Block);
        prev->next = block->next;
    }
}