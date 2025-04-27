#include <kernel/arch/i386/paging.h>
#include <stdio.h>

extern uint32_t pagedir[1024];
extern uint32_t pagedirs[MAX_PROCESSES][1024];
extern uint32_t pagetable[1024];
extern uint32_t pagetables_pagetable[1024];
extern void* _kernel_end_phys;

PagingState paging_state = {
    .pagetable_bitmap = {0},
    .pagedir_bitmap = {0},
    .total_pages = 0,
    .total_pagetables = 0,
    .used_pagetables = 0,
    .used_pagedirs = 0,
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

    // we hold pagedirs of processes inside pagedirs
    // we make sure they are initilized with 0
    for(int i=0; i< MAX_PROCESSES; i++){
        for(int j=0; j< 1024; j++){
            pagedirs[i][j]=0;
        }
    }
}

static inline uint32_t* get_pagetable_phys_byidx(int pt_idx){
    return paging_state.pagetables_phys_start + pt_idx * PAGE_SIZE;    
}

static inline uint32_t* get_pagedir_phys_byidx(int pd_idx){
    // return pagedirs - KERNEL_VBASE + pd_idx * PAGE_SIZE; // this should do the same   
    return (&pagedirs[pd_idx][0]) - KERNEL_VBASE;    
}


void switch_page_directory(uint32_t* new_pagedir_phys) {
    asm volatile("mov %0, %%cr3" : : "r" (new_pagedir_phys));
}

uint32_t* get_current_page_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    return (uint32_t*)cr3; // Physical address
}

uint32_t* get_pagedir_virtaddr(uint32_t phys_addr) {
    if(phys_addr  & 0x3FF) return 0; // not 4k aligned
    uint32_t min_phys_addr = pagedirs - KERNEL_VBASE;
    uint32_t max_phys_addr = pagedirs + MAX_PROCESSES - KERNEL_VBASE;
    if(phys_addr < min_phys_addr || phys_addr>max_phys_addr) return 0;

    return (uint32_t*) phys_addr + KERNEL_VBASE;
}

uint32_t* create_process_pd() {
    // Check if enough page tables are available (90% cap)
    if (paging_state.used_pagetables * 10 > paging_state.total_pagetables * 9) {
        return NULL; // Not enough free page tables for process mappings
    }

    // Find free page directory index
    int pd_idx = get_free_pagedir_idx();
    if (pd_idx < 0) return NULL; // No free page directories

    // Get physical and virtual addresses
    uint32_t* new_pd_phys = get_pagedir_phys_byidx(pd_idx);
    uint32_t* new_pd = get_pagedir_virtaddr((uint32_t)new_pd_phys);
    if (!new_pd) return NULL; // Invalid virtual address mapping

    // Clear page directory
    for (int i = 0; i < 1024; i++) {
        new_pd[i] = 0;
    }

    // Copy kernel mappings (0xC0000000-0xFFFFFFFF)
    for (int i = 768; i < 1024; i++) {
        if (pagedir[i] & PAGE_PRESENT) {
            new_pd[i] = pagedir[i];
        }
    }

    return new_pd_phys;
}

int assign_page_table(uint32_t* page_dir, void* virt_addr, uint32_t flags) {
    if ((uint32_t)virt_addr & 0x3FFFFF) return ASSIGN_PAGE_NOT_ALIGNED;
    uint32_t pd_idx = (uint32_t)virt_addr >> 22;
    if (pd_idx >= 768 && pd_idx < 1023) printf(" pd_idx: %08X vaddr: %08X ", pd_idx, virt_addr);//return ASSIGN_PAGE_KERNEL_SPACE;

    uint32_t* pd = get_pagedir_virtaddr((uint32_t)page_dir);
    if (pd[pd_idx] & PAGE_PRESENT) return ASSIGN_PAGE_ALREADY_MAPPED;

    int pt_idx = get_free_pagetable_idx();
    if (pt_idx < 0) return ASSIGN_PAGE_NO_TABLES;

    uint32_t pt_phys = get_pagetable_phys_byidx(pt_idx);
    pd[pd_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);    

    uint32_t* current_pd = get_current_page_directory();
    if (page_dir == current_pd) {
        asm volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
    }

    return ASSIGN_PAGE_OK;
}

int get_free_pagedir_idx(){
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!(paging_state.pagedir_bitmap[i / 8] & (1 << (i % 8)))) {
            paging_state.pagedir_bitmap[i / 8] |= (1 << (i % 8));
            paging_state.used_pagedirs++;
            return i;
        }
    }
    return -1;
}

void free_pagedir_idx(int idx) {
    if (idx < 0 || idx >= MAX_PROCESSES) {
        printf("Invalid pagedir index: %d\n", idx);
        return;
    }
    if (!(paging_state.pagedir_bitmap[idx / 8] & (1 << (idx % 8)))) {
        printf("pagedir %d already free\n", idx);
        return;
    }

    paging_state.pagedir_bitmap[idx / 8] &= ~(1 << (idx % 8));
    paging_state.used_pagedirs--;
}

FreePageStatus free_pagedir_by_vaddr(void* vaddr) {
    uint32_t addr = (uint32_t)vaddr;
    uint32_t pagedirs_addr = (uint32_t)pagedirs;
    
    if (addr < pagedirs_addr || addr >= pagedirs_addr + MAX_PROCESSES) {
        return FREE_PAGE_OUT_OF_RANGE;
    }
    if (addr & (PAGE_SIZE - 1)) {
        return FREE_PAGE_NOT_ALIGNED;
    }

    int idx = (addr - pagedirs_addr) / PAGE_SIZE;
    if (idx >= MAX_PROCESSES) {
        return FREE_PAGE_EXCEEDS_POOL;
    }

    free_pagedir_idx(idx);
    return FREE_PAGE_OK;
}

int get_free_pagetable_idx() {
    for (int i = 0; i < paging_state.total_pagetables; i++) {
        if (!(paging_state.pagetable_bitmap[i / 8] & (1 << (i % 8)))) {
            paging_state.pagetable_bitmap[i / 8] |= (1 << (i % 8));
            paging_state.used_pagetables++;
            return i;
        }
    }
    return -1;
}

void free_pagetable_idx(int idx) {
    if (idx < 0 || idx >= paging_state.total_pagetables) {
        printf("Invalid pagetable index: %d\n", idx);
        return;
    }
    if (!(paging_state.pagetable_bitmap[idx / 8] & (1 << (idx % 8)))) {
        printf("Pagetable %d already free\n", idx);
        return;
    }

    paging_state.pagetable_bitmap[idx / 8] &= ~(1 << (idx % 8));
    paging_state.used_pagetables--;
}

FreePageStatus free_pagetable_by_vaddr(void* vaddr) {
    uint32_t addr = (uint32_t)vaddr;
    if (addr < paging_state.pagetables_virt_start || addr >= 0xFFFFFFFF) {
        return FREE_PAGE_OUT_OF_RANGE;
    }
    if (addr & (PAGE_SIZE - 1)) {
        return FREE_PAGE_NOT_ALIGNED;
    }

    int idx = (addr - paging_state.pagetables_virt_start) / PAGE_SIZE;
    if (idx >= paging_state.total_pagetables) {
        return FREE_PAGE_EXCEEDS_POOL;
    }

    free_pagetable_idx(idx);
    return FREE_PAGE_OK;
}