#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include "multiboot.h"

#define PAGE_SIZE 4096
#define KERNEL_VBASE 0xC0000000

// Page table entry flags
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)

// Initialize paging with Multiboot memory info
void init_paging(multiboot_info_t* mbi);

// Allocate a physical page, return virtual address in kernel space
void* alloc_page();

// Free a previously allocated page
void free_page(void* page);

// Map a physical address to a virtual address in the current page directory
int map_page(void* phys_addr, void* virt_addr, uint32_t flags);

// Unmap a virtual address
void unmap_page(void* virt_addr);

// Switch to a new page directory (for process isolation)
void switch_page_directory(uint32_t* new_pagedir_phys);

// Get the current page directory's physical address
uint32_t* get_current_page_directory();

// Handle a page fault (called from interrupt handler)
void page_fault_handler(uint32_t error_code, void* fault_addr);

#endif