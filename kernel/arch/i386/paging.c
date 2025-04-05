#include <kernel/paging.h>

extern uint32_t pagedir[1024];
extern uint32_t pagetable[1024];
extern void* _kernel_end;

// Physical memory limits from Multiboot
static uint32_t mem_lower;  // Below 1MB (not used here)
static uint32_t mem_upper;  // Above 1MB in KB
static uint32_t total_pages;
static uint32_t* page_bitmap;  // Dynamic bitmap
static uint32_t bitmap_size;

void init_paging(multiboot_info_t* mbi) {
    // Use Multiboot memory map for accuracy
    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
        total_pages = (16 * 1024 * 1024) / PAGE_SIZE;  // Fallback: 16MB
    } else {
        multiboot_memory_map_t* mmap = (void*)mbi->mmap_addr;
        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
        uint32_t max_addr = 0;
        while ((uint32_t)mmap < mmap_end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->addr == 0x100000) {
                max_addr = mmap->addr + mmap->len;
            }
            mmap = (void*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
        total_pages = max_addr / PAGE_SIZE;
    }

    bitmap_size = (total_pages + 31) / 32;
    page_bitmap = (uint32_t*)((uint32_t)&_kernel_end + KERNEL_VBASE);
    uint32_t bitmap_bytes = bitmap_size * sizeof(uint32_t);
    uint32_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // Mark used pages (0x0 - _kernel_end + bitmap)
    uint32_t kernel_end_page = ((uint32_t)&_kernel_end - KERNEL_VBASE) / PAGE_SIZE;
    for (uint32_t i = 0; i < kernel_end_page + bitmap_pages; i++) {
        page_bitmap[i / 32] |= (1 << (i % 32));
    }
}

void* alloc_page() {
    for (uint32_t i = 0; i < bitmap_size; i++) {
        if (page_bitmap[i] != 0xFFFFFFFF) {
            for (uint8_t bit = 0; bit < 32; bit++) {
                if (!(page_bitmap[i] & (1 << bit))) {
                    uint32_t page_idx = i * 32 + bit;
                    page_bitmap[i] |= (1 << bit);
                    uint32_t phys_addr = page_idx * PAGE_SIZE;
                    void* virt_addr = (void*)(KERNEL_VBASE + phys_addr);
                    if (phys_addr >= 0x400000) {
                        map_page((void*)phys_addr, virt_addr, PAGE_PRESENT | PAGE_WRITE);
                    }
                    return virt_addr;
                }
            }
        }
    }
    return 0;
}

void free_page(void* page) {
    uint32_t virt_addr = (uint32_t)page;
    uint32_t phys_addr = virt_addr - KERNEL_VBASE;
    uint32_t page_idx = phys_addr / PAGE_SIZE;
    if (page_idx < total_pages) {
        page_bitmap[page_idx / 32] &= ~(1 << (page_idx % 32));
        // Optionally unmap if outside initial 4MB
        if (phys_addr >= 0x400000) {
            unmap_page((void*)virt_addr);
        }
    }
}

int map_page(void* phys_addr, void* virt_addr, uint32_t flags) {
    uint32_t pd_idx = (uint32_t)virt_addr >> 22;         // Top 10 bits
    uint32_t pt_idx = ((uint32_t)virt_addr >> 12) & 0x3FF; // Next 10 bits

    uint32_t* pd = get_current_page_directory();
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        // Allocate a new page table
        void* new_pt_phys = alloc_page();
        if (!new_pt_phys) return -1;  // Out of memory
        pd[pd_idx] = (uint32_t)new_pt_phys - KERNEL_VBASE | PAGE_PRESENT | PAGE_WRITE;
        // Zero it
        uint32_t* new_pt = (uint32_t*)new_pt_phys;
        for (int i = 0; i < 1024; i++) new_pt[i] = 0;
    }

    uint32_t* pt = (uint32_t*)((pd[pd_idx] & ~0xFFF) + KERNEL_VBASE);
    pt[pt_idx] = (uint32_t)phys_addr | flags;
    asm volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");  // Invalidate TLB
    return 0;
}

void unmap_page(void* virt_addr) {
    uint32_t pd_idx = (uint32_t)virt_addr >> 22;
    uint32_t pt_idx = ((uint32_t)virt_addr >> 12) & 0x3FF;

    uint32_t* pd = get_current_page_directory();
    if (pd[pd_idx] & PAGE_PRESENT) {
        uint32_t* pt = (uint32_t*)((pd[pd_idx] & ~0xFFF) + KERNEL_VBASE);
        pt[pt_idx] = 0;
        asm volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
    }
}

void switch_page_directory(uint32_t* new_pagedir_phys) {
    asm volatile("mov %0, %%cr3" : : "r" (new_pagedir_phys));
}

uint32_t* get_current_page_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    return (uint32_t*)(cr3 + KERNEL_VBASE);  // Virtual address
}

void page_fault_handler(uint32_t error_code, void* fault_addr) {
    // Simple handler for now
    volatile uint32_t* vga = (uint32_t*)0xC00B8000;
    *vga = 0x0F500F50;  // "PP" for page fault
    vga[1] = (error_code & 0x1) ? 0x0F450F45 : 0x0F4E0F4E;  // "EE" present, "NN" not present
    while (1) asm("hlt");
}