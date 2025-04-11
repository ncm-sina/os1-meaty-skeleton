#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <kernel/multiboot.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_VBASE 0xC0000000
#define PAGETABLES_VIRT_START 0xFFC00000
#define PAGETABLE_BITMAP_SIZE (128) /* 128 bytes = 1024 bits for 4GB */

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)

extern uint32_t pagedir[1024];
extern uint32_t pagetable[1024];
extern uint32_t pagetables_pagetable[1024];
extern void* _kernel_end_phys;

typedef struct {
    uint8_t pagetable_bitmap[PAGETABLE_BITMAP_SIZE];
    int32_t total_pages;
    int32_t total_pagetables;
    int32_t used_pagetables;
    uint32_t pagetables_phys_start;
    uint32_t pagetables_virt_start;
} PagingState;

extern PagingState paging_state;

void init_paging(multiboot_info_t* mbi);
void switch_page_directory(uint32_t* new_pagedir_phys);
uint32_t* get_current_page_directory();
uint32_t* create_process_pd();
int assign_page_table(uint32_t* page_dir, void* virt_addr, uint32_t flags);
void free_pagetable_idx(int idx);
int free_pagetable_by_vaddr(void* vaddr);

#endif