#include <kernel/paging.h>
#include <kernel/mconio.h>

extern uint32_t pagedir[1024];
extern uint32_t pagetable[1024];
extern uint32_t pagetables_pagetable[1024];
extern void* _kernel_end_phys;

PagingState paging_state = {
    .pagetable_bitmap = {0},
    .total_pages = 0,
    .total_pagetables = 0,
    .used_pagetables = 0,
    .pagetables_phys_start = 0,
    .pagetables_virt_start = PAGETABLES_VIRT_START
};

void init_paging(multiboot_info_t* mbi) {
    // Calculate total pages from Multiboot memory map
    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
        paging_state.total_pages = (16 * 1024 * 1024) / PAGE_SIZE; // Fallback: 16MB
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
        paging_state.total_pages = max_addr / PAGE_SIZE;
    }

    paging_state.total_pagetables = paging_state.total_pages / 1024;
    if (paging_state.total_pagetables > 1024) paging_state.total_pagetables = 1024;

    // Set physical start of page tables after kernel
    paging_state.pagetables_phys_start = (_kernel_end_phys > 0x400000 ? _kernel_end_phys : 0x400000);
    paging_state.pagetables_phys_start = (paging_state.pagetables_phys_start + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

    // Map pagetables_virt_start (0xFFC00000) to pagetables_phys_start
    for (int i = 0; i < 1024; i++) {
        pagetables_pagetable[i] = (paging_state.pagetables_phys_start + i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }
    pagedir[1023] = ((uint32_t)pagetables_pagetable - KERNEL_VBASE) | PAGE_PRESENT | PAGE_WRITE;

    // Initialize page tables at pagetables_virt_start
    uint32_t* tmp_pagetable;
    for (int i = 0; i < paging_state.total_pagetables; i++) {
        tmp_pagetable = (uint32_t*)(paging_state.pagetables_virt_start + i * PAGE_SIZE);
        for (int j = 0; j < 1024; j++) {
            tmp_pagetable[j] = ((i * 1024 + j) * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
        }
    }

    // Initialize pagetable bitmap
    paging_state.used_pagetables = 0;
    uint32_t pagetables_phys_end = paging_state.pagetables_phys_start + 0x400000;
    uint32_t first_free_pagetable = (pagetables_phys_end + (1024 * PAGE_SIZE) - 1) / (1024 * PAGE_SIZE);
    for (int i = 0; i < first_free_pagetable; i++) {
        paging_state.pagetable_bitmap[i / 8] |= (1 << (i % 8));
        paging_state.used_pagetables++;
    }
}

void switch_page_directory(uint32_t* new_pagedir_phys) {
    asm volatile("mov %0, %%cr3" : : "r" (new_pagedir_phys));
}

uint32_t* get_current_page_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    return (uint32_t*)cr3; // Physical address
}

static uint32_t* get_virt_pointer(uint32_t phys_addr) {
    uint32_t* pt = (uint32_t*)0xC0104000; // pagetable's virtual address
    uint32_t temp_idx = (paging_state.pagetables_virt_start >> 12) & 0x3FF; // PTE for 0xFFC00000
    pt[temp_idx] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    asm volatile("invlpg (%0)" : : "r" (paging_state.pagetables_virt_start) : "memory");
    return (uint32_t*)paging_state.pagetables_virt_start;
}

uint32_t* create_process_pd() {
    uint32_t pd_idx = 0;
    for (int i = 0; i < paging_state.total_pagetables; i++) {
        if (!(paging_state.pagetable_bitmap[i / 8] & (1 << (i % 8)))) {
            paging_state.pagetable_bitmap[i / 8] |= (1 << (i % 8));
            pd_idx = i;
            paging_state.used_pagetables++;
            break;
        }
    }
    if (pd_idx == 0) return NULL; // No free page tables (0 is reserved)

    uint32_t* new_pd_phys = (uint32_t*)(paging_state.pagetables_phys_start + pd_idx * PAGE_SIZE);
    uint32_t* new_pd = get_virt_pointer((uint32_t)new_pd_phys);

    for (int i = 0; i < 1024; i++) {
        new_pd[i] = 0;
    }
    for (int i = 768; i < 1024; i++) {
        if (pagedir[i] & PAGE_PRESENT) {
            new_pd[i] = pagedir[i];
        }
    }

    return new_pd_phys;
}

static int get_free_pagetable_idx() {
    for (int i = 0; i < paging_state.total_pagetables; i++) {
        if (!(paging_state.pagetable_bitmap[i / 8] & (1 << (i % 8)))) {
            paging_state.pagetable_bitmap[i / 8] |= (1 << (i % 8));
            paging_state.used_pagetables++;
            return i;
        }
    }
    return -1;
}

int assign_page_table(uint32_t* page_dir, void* virt_addr, uint32_t flags) {
    if ((uint32_t)virt_addr & 0x3FFFFF) return -3; // Must be 4MB-aligned
    uint32_t pd_idx = (uint32_t)virt_addr >> 22;
    if (pd_idx >= 768 && pd_idx < 1023) return -4; // Kernel space

    uint32_t* pd = 0;//get_virt_pointer((uint32_t)page_dir);
    if (pd[pd_idx] & PAGE_PRESENT) return -2; // Already mapped

    int pt_idx = get_free_pagetable_idx();
    if (pt_idx < 0) return -1; // No free page tables

    uint32_t pt_phys = paging_state.pagetables_phys_start + pt_idx * PAGE_SIZE;
    pd[pd_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);

    uint32_t* current_pd = get_current_page_directory();
    if (page_dir == current_pd) {
        asm volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
    }

    return 0;
}

void free_pagetable_idx(int idx) {
    if (idx < 0 || idx >= paging_state.total_pagetables) {
        cprintf("Invalid pagetable index: %d\n", idx);
        return;
    }
    if (!(paging_state.pagetable_bitmap[idx / 8] & (1 << (idx % 8)))) {
        cprintf("Pagetable %d already free\n", idx);
        return;
    }

    paging_state.pagetable_bitmap[idx / 8] &= ~(1 << (idx % 8));
    paging_state.used_pagetables--;
}

int free_pagetable_by_vaddr(void* vaddr) {
    uint32_t addr = (uint32_t)vaddr;
    if (addr < paging_state.pagetables_virt_start || addr >= 0xFFFFFFFF) {
        return -1; // Out of pool range
    }
    if (addr & (PAGE_SIZE - 1)) {
        return -2; // Not aligned
    }

    int idx = (addr - paging_state.pagetables_virt_start) / PAGE_SIZE;
    if (idx >= paging_state.total_pagetables) {
        return -3; // Beyond allocated tables
    }

    free_pagetable_idx(idx);
    return 0;
}