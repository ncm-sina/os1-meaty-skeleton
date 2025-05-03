#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <kernel/multiboot.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_VBASE 0xC0000000

#ifndef MAX_PROCESSES
    #define MAX_PROCESSES 256
#endif

#define PAGETABLES_VIRT_START 0xFFC00000
#define PAGETABLE_BITMAP_SIZE (128)
#define PAGEDIR_BITMAP_SIZE (MAX_PROCESSES / 8)

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)


extern uint32_t pagedir[1024];
extern uint32_t pagetable[1024];
extern uint32_t pagetables_pagetable[1024];
extern void* _kernel_end_phys;

typedef struct {
    uint8_t pagetable_bitmap[PAGETABLE_BITMAP_SIZE];
    uint8_t pagedir_bitmap[PAGEDIR_BITMAP_SIZE];
    int32_t total_pages;
    int32_t total_pagetables;
    int32_t used_pagetables;
    int32_t used_pagedirs;
    uint32_t pagetables_phys_start;
    uint32_t pagetables_virt_start;
} PagingState;

extern PagingState paging_state;

// Status codes for assign_page_table
typedef enum {
    ASSIGN_PAGE_OK = 0,           // Success
    ASSIGN_PAGE_NO_TABLES = -1,   // No free page tables
    ASSIGN_PAGE_ALREADY_MAPPED = -2, // PDE already mapped
    ASSIGN_PAGE_NOT_ALIGNED = -3, // Virtual address not 4MB-aligned
    ASSIGN_PAGE_KERNEL_SPACE = -4 // Attempt to map in kernel space
} AssignPageStatus;

// Status codes for free_pagetable_by_vaddr
typedef enum {
    FREE_PAGE_OK = 0,           // Success
    FREE_PAGE_OUT_OF_RANGE = -1, // Address outside pool range
    FREE_PAGE_NOT_ALIGNED = -2,  // Not 4KB-aligned
    FREE_PAGE_EXCEEDS_POOL = -3  // Index beyond total_pagetables
} FreePageStatus;


// Function declarations
void init_paging(multiboot_info_t* mbi);
void switch_page_directory(uint32_t* new_pagedir_phys);
uint32_t* get_current_page_directory(void);
uint32_t* get_pagedir_virtaddr(uint32_t phys_addr);
uint32_t* create_process_pd(void);
int assign_page_table(uint32_t* page_dir, void* virt_addr, uint32_t flags);
int get_free_pagedir_idx(void);
void free_pagedir_idx(int idx);
FreePageStatus free_pagedir_by_vaddr(void* vaddr);
int get_free_pagetable_idx(void);
void free_pagetable_idx(int idx);
FreePageStatus free_pagetable_by_vaddr(void* vaddr);

// void init_paging(multiboot_info_t* mbi);
// void switch_page_directory(uint32_t* new_pagedir_phys);
// uint32_t* get_current_page_directory();
// uint32_t* create_process_pd();
// AssignPageStatus assign_page_table(uint32_t* page_dir, void* virt_addr, uint32_t flags);
// void free_pagetable_idx(int idx);
// FreePageStatus free_pagetable_by_vaddr(void* vaddr);
// void free_pagedir_idx(int idx);
// FreePageStatus free_pagedir_by_vaddr(void* vaddr);

#endif