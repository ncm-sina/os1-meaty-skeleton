#include <kernel/paging.h>
#include <kernel/mconio.h>

extern uint32_t pagedir[1024];     // From boot.S
extern uint32_t pagetable[1024];   // From boot.S
extern void* _kernel_end_phys;

uint32_t template_page_dir[1024] __attribute__((aligned(4096)));
uint32_t* phys_bitmap;
uint32_t* virt_bitmap;

uint32_t bitmap_size;
uint32_t total_pages;
static uint32_t _next_kernel_offset = 0xC0100000;

void print_bitmaps(int a, int b){
    int i=0;
    cprintf("bitmaps:");
    for(i=a; i<bitmap_size && i<b; i++){
        if (i%5 == 0) cprintf("\n");
        cprintf("%08X ", phys_bitmap[i]);
    }
}

uint32_t get_free_kernel_vaddr(){
    _next_kernel_offset = (_next_kernel_offset + PAGE_SIZE -1) / PAGE_SIZE * PAGE_SIZE;
    _next_kernel_offset += PAGE_SIZE;
    return _next_kernel_offset - PAGE_SIZE;
}

void init_paging(multiboot_info_t* mbi) {
    // Step 1: Calculate total pages from Multiboot memory map
    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
        total_pages = (16 * 1024 * 1024) / PAGE_SIZE; // Fallback: 16MB
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

    _next_kernel_offset = &_kernel_end_phys > 0x100000? &_kernel_end_phys : 0x100000;
    _next_kernel_offset += KERNEL_VBASE;
    _next_kernel_offset = (_next_kernel_offset + PAGE_SIZE -1) / PAGE_SIZE * PAGE_SIZE;
    // from now on use get_free_kernel_vaddr() to get empty virtual address in kernelspace

    // Step 2: Initialize physical bitmap
    bitmap_size = (total_pages + 31) / 32; // Number of 32-bit words
    phys_bitmap = (uint32_t*)((uint32_t) ((int)&_kernel_end_phys + PAGE_SIZE -1) / PAGE_SIZE * PAGE_SIZE); // next empty pysical memory (aligned 4k)
    uint32_t bitmap_bytes = bitmap_size * sizeof(uint32_t);
    uint32_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    // cprintf("total_page: %08X bitmap_size: %08X bitmap_bytes: %08X bitmap_pages: %08X ", total_pages, bitmap_size, bitmap_bytes, bitmap_pages);

    // Map bitmap pages using the initial pagedir from boot.S
    for (uint32_t i = 0; i < bitmap_pages; i++) {
        uint32_t phys_addr = ((uint32_t)phys_bitmap) + i * PAGE_SIZE;
        // cprintf("phys_addr: %08x virt_addr: %08X |\n", phys_addr , _next_kernel_offset);
        map_page(pagedir, (void*)phys_addr, (void*)((uint32_t)get_free_kernel_vaddr()), 
                 PAGE_PRESENT | PAGE_WRITE);
        _next_kernel_offset+= PAGE_SIZE;
    }
    
    // Zero the bitmap
    for (uint32_t i = 0; i < bitmap_size; i++) {
        phys_bitmap[i] = 0;
    }    
    
    // Mark used pages (0x0 to _kernel_end_phys + bitmap)
    uint32_t kernel_end_page = (((uint32_t)&_kernel_end_phys + PAGE_SIZE - 1) / PAGE_SIZE) + bitmap_pages;
    for (uint32_t i = 0; i < kernel_end_page; i++) {
        phys_bitmap[i / 32] |= (1 << (i % 32));
    }
    
    print_bitmaps(0,20);
    while(1);

    // Step 3: Set up template_page_dir
    for (int i = 0; i < 1024; i++) {
        template_page_dir[i] = 0; // Clear all entries
    }

    // Share kernel mappings from boot.S's pagedir
    template_page_dir[0] = pagedir[0];     // 0x0 - 0x400000
    template_page_dir[768] = pagedir[768]; // 0xC0000000 - 0xC0400000

    // Step 4: Switch to template_page_dir as the active directory
    // switch_page_directory((uint32_t*)((uint32_t)template_page_dir - KERNEL_VBASE));
}

void* alloc_phys_page() {
    for (uint32_t i = 0; i < bitmap_size; i++) {
        if (phys_bitmap[i] != 0xFFFFFFFF) {
            for (uint8_t bit = 0; bit < 32; bit++) {
                if (!(phys_bitmap[i] & (1 << bit))) {
                    uint32_t page_idx = i * 32 + bit;
                    phys_bitmap[i] |= (1 << bit);
                    return (void*)(page_idx * PAGE_SIZE); // Physical address
                }
            }
        }
    }
    return NULL; // Out of memory
}

void free_phys_page(void* phys_addr) {
    uint32_t page_idx = (uint32_t)phys_addr / PAGE_SIZE;
    if (page_idx < total_pages) {
        phys_bitmap[page_idx / 32] &= ~(1 << (page_idx % 32));
    }
}

int map_page(uint32_t* page_dir, void* phys_addr, void* virt_addr, uint32_t flags) {
    uint32_t pd_idx = (uint32_t)virt_addr >> 22;
    uint32_t pt_idx = ((uint32_t)virt_addr >> 12) & 0x3FF;

    uint32_t* pd = (uint32_t*)((uint32_t)page_dir + KERNEL_VBASE); // Virtual address
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        void* new_pt_phys = alloc_phys_page();
        if (!new_pt_phys) return -1; // Out of memory
        pd[pd_idx] = (uint32_t)new_pt_phys | PAGE_PRESENT | PAGE_WRITE;
        uint32_t* new_pt = (uint32_t*)((uint32_t)new_pt_phys + KERNEL_VBASE);
        for (int i = 0; i < 1024; i++) new_pt[i] = 0; // Zero new page table
    }

    uint32_t* pt = (uint32_t*)((pd[pd_idx] & ~0xFFF) + KERNEL_VBASE);
    if (pt[pt_idx] & PAGE_PRESENT) return -2; // Already mapped
    pt[pt_idx] = (uint32_t)phys_addr | flags;
    asm volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
    return 0;
}

void switch_page_directory(uint32_t* new_pagedir_phys) {
    asm volatile("mov %0, %%cr3" : : "r" (new_pagedir_phys));
}

uint32_t* get_current_page_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    return (uint32_t*)(cr3 + KERNEL_VBASE); // Virtual address
}

uint32_t* create_process_pd() {
    void* new_pd_phys = alloc_phys_page();
    if (!new_pd_phys) return NULL;
    uint32_t* new_pd = (uint32_t*)((uint32_t)new_pd_phys + KERNEL_VBASE);

    // Clear the new page directory
    for (int i = 0; i < 1024; i++) {
        new_pd[i] = 0;
    }

    // Share kernel mappings directly from template_page_dir
    for (int i = 768; i < 1024; i++) {
        if (template_page_dir[i] & PAGE_PRESENT) {
            new_pd[i] = template_page_dir[i]; // Copy PDE, sharing page tables
        }
    }

    return new_pd_phys; // Return physical address
}