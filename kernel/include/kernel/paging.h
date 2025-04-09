#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <kernel/multiboot.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_VBASE 0xC0000000

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)

extern uint32_t template_page_dir[1024];
extern uint32_t* phys_bitmap;
extern uint32_t bitmap_size;
extern uint32_t total_pages;

void init_paging(multiboot_info_t* mbi);
void* alloc_phys_page();
void free_phys_page(void* phys_addr);
int map_page(uint32_t* page_dir, void* phys_addr, void* virt_addr, uint32_t flags);
void switch_page_directory(uint32_t* new_pagedir_phys);
uint32_t* create_process_pd();
uint32_t* get_current_page_directory();

#endif