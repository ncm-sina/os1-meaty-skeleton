#include <kernel/arch/i386/paging.h>
#include <stdio.h>

// extern uint32_t pagedirs[MAX_PROCESSES][1024];
// extern uint32_t pagetable[1024];
// extern uint32_t pagetables_pagetable[1024];

#define KERNEL_HEAP_SIZE 0x1000000 // 16 MB


PagingState paging_state = {
    .total_pages = 0,

    .mapped_phys_page_bitmap = {0},
    .mapped_phys_page_bitmap_size = 0,

    .pagetable_bitmap = {0},
    .pagedir_bitmap = {0},
    .total_pagetables = 0,
    .used_pagetables = 0,
    .used_pagedirs = 0,
    .pagetables_phys_start = 0,
    .pagetables_virt_start = PAGETABLES_VIRT_START
};



void print_mmap_addresses(multiboot_info_t* mbi){

        
    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
        printf("\n info mem map not available\n");
    } else {
        multiboot_memory_map_t* mmap = (void*)mbi->mmap_addr;
        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
        uint32_t max_addr = 0;
        printf("\n mbi address: %#08X", mmap);
        printf("\n #   size\t addr\tend\tlength\ttype\n");
        uint16_t i=0;
        while ((uint32_t)mmap < mmap_end) {
            printf(" %d: %#08x %#08X %#08X %#08X %#08x\n", ++i,
                mmap->size,
                mmap->addr,
                mmap->addr+mmap->len,
                mmap->len,
                mmap->type
                // ((uint32_t)mmap->addr)+((uint32_t) mmap->len),
            );
               mmap = (void*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
        }
}

void print_module_info(multiboot_info_t* mbi) {
    if (!(mbi->flags & MULTIBOOT_INFO_MODS)) {
        printf("\nModule information not available\n");
        return;
    }

    if (mbi->mods_count == 0) {
        printf("\nNo modules loaded\n");
        return;
    }

    multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
    printf("\n #\t   Start\t   End\t\t   Cmdline\n");
    
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        // Get module fields
        uint32_t start = mods[i].mod_start;
        uint32_t end = mods[i].mod_end;
        char* cmdline = (char*)mods[i].cmdline;

        // Print module info (handle null or invalid cmdline)
        printf(" %u:\t%#08x\t%#08x\t%s\n", i, start, end, 
               (cmdline && mods[i].cmdline != 0) ? cmdline : "(none)");
    }
}

uint32_t get_free_pagetable(uint8_t flush){
    printf(" 2:%08x ",&last_pagetable_idx);
    if(last_pagetable_idx >= PAGETABLES_MAXSIZE){
        printf("\n no free pagetable ");
        return ~0; // no free pagetable left
    }

    printf("pt: %08x %08x", (uint32_t)pagetables, last_pagetable_idx);

    page_table_t *low_mem_pagetable = (page_table_t *) (((uint32_t)pagetables) - KERNEL_VBASE);

    page_table_t *pt = &low_mem_pagetable[last_pagetable_idx++];
    printf("lmpt:%08x pts:%08x pt:%08x li:%d\n", low_mem_pagetable, pagetables, pt, last_pagetable_idx);
    printf(" 4 ");
    if(flush){
        for(uint32_t i=0; i< PAGE_TABLE_SIZE; i++)
            pt->entries[i] = 0;
    }
    printf(" 5 ");
    return (uint32_t) pt;
}

static inline void calculate_total_pages(multiboot_info_t* mbi){
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
}

static inline void calculate_bitmap_size_and_pagetables(){
    paging_state.mapped_phys_page_bitmap_size = (paging_state.total_pages + 31) / 32;
    // calculate bitmap size and its pagetable(s)
    // bitmap_size = (total_pages + 31) / 32;
    // paging_state.mapped_phys_page_bitmap = (uint32_t*)((uint32_t)&_kernel_end_virt);
    // uint32_t bitmap_bytes = bitmap_size * sizeof(uint32_t);
    // uint32_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
}

static int phys_page_already_mapped(uint32_t phys_page){
    uint16_t bitmap_index = phys_page/32;
    if(bitmap_index<0 || bitmap_index > M_PHYS_P_BITMAP_MAX_SIZE){
        printf(" phys page out of range!\n");
        return -1;
    }
    return paging_state.mapped_phys_page_bitmap[bitmap_index] & (1 << (phys_page%32)); 
}

static inline void set_phys_page_mapped(uint32_t phys_page){
    uint16_t bitmap_index = phys_page /32;
    if(bitmap_index<0 || bitmap_index > M_PHYS_P_BITMAP_MAX_SIZE){
        printf(" phys page out of range!\n");
        return -1;
    }
    paging_state.mapped_phys_page_bitmap[bitmap_index] |= (1 << (phys_page % 32));
}

static int mappage_phys_to_virt(page_directory_t *page_dir, uint32_t phys_page, uint32_t virt_page, uint32_t flags, uint8_t force){
    if(!force && (phys_page_already_mapped(phys_page) == 1)){
        printf(" Physical page already mapped. failed: pd:%08x pp:%08x vp:%08x fl:%08x ", page_dir, phys_page, virt_page, flags);
        return -1;
    }

    uint16_t pd_idx = virt_page / 1024;
    uint16_t pt_idx = virt_page % 1024;

    printf("\033[33m mappage_phys_to_virt: %08x %08x %08x %08x %08x \033[37m",page_dir, phys_page, virt_page, pd_idx, pt_idx);

    if(!(page_dir->entries[pd_idx] & PAGE_PRESENT)){
        // printf(" 1 ");
        // reason why we can use -KERNEL_VBASE and +KERNEL_VBASE is that page dirs and page tables are stored
        // in high half memory which is mapped to virt_addr - KERNEL_VBASE
        //create page table and assign it to page_dir[pd_idx]
        // page_dir[pd_idx] stores address of the corresponding page table
        // we can ignore KERNEL_VBASE and use page tables' phys address as virtual address
        // because they are identity mapped otherwise we need to do +- KERNEL_VBASE
        uint32_t tmppt = (uint32_t)get_free_pagetable(1);
        // printf(" jjj ");
        printf(" not present get a free pt: %08x ", tmppt);
        if(tmppt == ~0){
            printf("\n no free pt ");
            return -2;
        }
        page_dir->entries[pd_idx] =   tmppt/*- KERNEL_VBASE*/ | PAGE_PRESENT | PAGE_WRITE;
        // new_pt points to that page table we want to make sure that the page table is empty

        // passing non zero value to get_free_pagetable will give us flushed page table
        // so we dont need to flush it
        // page_table_t* new_pt = (page_table_t*) (page_dir->entries[pd_idx] /*+ KERNEL_VBASE*/);
        // for (int i = 0; i < 1024; i++) new_pt->entries[i] = 0; //todo: we can free those page while we set them to 0
    }else{
        printf(" page present %08x ",page_dir->entries[pd_idx]);
    }

    page_table_t* pt = (page_table_t*)((page_dir->entries[pd_idx] & ~0xFFF) /*+ KERNEL_VBASE*/);
    if(pt->entries[pt_idx]){
        // todo if no virt page is pointing to that phys page then free that page
    }
    set_phys_page_mapped(phys_page);
    pt->entries[pt_idx] = (phys_page * PAGE_SIZE) | flags;
    return 0;
}

int map_phys_to_virt(page_directory_t* page_dir, uint32_t phys_addr, uint32_t virt_addr, uint32_t flags, uint8_t force){
    if( (phys_addr & 0xFFF) || (virt_addr & 0xFFF) )
        return -2; // addresses must be 4k alligned

    // printf("\033[32mmap_phys_to_virt: %08x %08x %08x %08x %08x \033[37m",page_dir, phys_addr, virt_addr, flags, force);

    uint32_t phys_page = phys_addr/PAGE_SIZE;
    uint32_t virt_page = virt_addr/PAGE_SIZE;
    return mappage_phys_to_virt(page_dir, phys_page, virt_page, flags, force);
}


static inline uint32_t get_last_module_phys_addr(multiboot_info_t* mbi){
    uint32_t ret=0;
    if (!(mbi->flags & MULTIBOOT_INFO_MODS)) {
        printf("\nModule information not available\n");
        return 0;
    }

    if (mbi->mods_count == 0) {
        printf("\nNo modules loaded\n");
        return 0;
    }

    multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
    
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        // Get module fields
        uint32_t end = mods[i].mod_end;
        if(end>ret)
            ret = end;
    }
    return ret;
}

static inline void mark_initial_used_physical_pages (multiboot_info_t* mbi){
    // zero out bitmap first
    for (uint16_t i = 0; i< M_PHYS_P_BITMAP_MAX_SIZE; i++){
        paging_state.mapped_phys_page_bitmap[i]=0;
    }
    printf("_kernel_end_phys: %08x ", &_kernel_end_phys);

    uint32_t reserved_mem_end_phys_addr = ((uint32_t)&_kernel_end_phys);
    // printf("reserved_mem_end_phys_addr1: %08x ", reserved_mem_end_phys_addr);

    // we mark all memory from start to end of last multiboot module as used and we make sure they are mapped and identity mapped
    // todo (we should only have first 1Mb identity mapped)
    uint32_t last_module_phys_addr = get_last_module_phys_addr(mbi);
    printf("last_module_phys_addr: %08x ", last_module_phys_addr);
    if(last_module_phys_addr> reserved_mem_end_phys_addr){
        reserved_mem_end_phys_addr = last_module_phys_addr;
    }
    printf("reserved_mem_end_phys_addr: %08x ", reserved_mem_end_phys_addr);
    uint32_t reserved_mem_end_page = (reserved_mem_end_phys_addr + 2*PAGE_SIZE - 1) / PAGE_SIZE;
    printf("reserved end page: %08x ", reserved_mem_end_page);

    for(uint32_t i=0; i< reserved_mem_end_page ; i++){
        paging_state.mapped_phys_page_bitmap[i/32] |= (1 << (i % 32));
    }

    printf(" mapped_phys_page_bitmap: %08x ", paging_state.mapped_phys_page_bitmap);
}


void enable_paging(page_directory_t* kpage_dir) {
    asm volatile(
        "movl %0, %%eax\n"          // Load kernel_pagedir into EAX
        "movl %%eax, %%cr3\n"       // Move EAX (kernel_pagedir) to CR3
        "movl %%cr0, %%eax\n"       // Load CR0 into EAX
        "orl $0x80000000, %%eax\n"  // Set paging bit (PG, bit 31)
        "movl %%eax, %%cr0\n"       // Write back to CR0 to enable paging
        "mov $1f, %%eax\n" // Load higher_half address into EAX
        "jmp *%%eax\n"              // Jump to higher_half
        "1:\n"            // Label for higher-half kernel
        :
        : "r"(kpage_dir)       // Input: kernel_pagedir as a register operand
        : "%eax", "cc"              // Clobbers: EAX and condition codes
    );
}

void init_paging_stage1(multiboot_info_t* mbi) {
    page_directory_t* kernel_pagedir_phys_addr = (page_directory_t*)( ((uint32_t)&kernel_pagedir) - KERNEL_VBASE);
    page_table_t* pagetables_phys_addr = (page_table_t*)( ((uint32_t)pagetables) - KERNEL_VBASE);
    uint32_t flags;
    uint32_t i;
    uint32_t reserved_mem_end_phys_addr = ((uint32_t)&_kernel_end_phys);

    // we mark all memory from start to end of last multiboot module as used and we make sure they are mapped and identity mapped
    // todo (we should only have first 1Mb identity mapped)
    uint32_t last_module_phys_addr = get_last_module_phys_addr(mbi);
    // printf("last_module_phys_addr: %08x ", last_module_phys_addr);
    if(last_module_phys_addr> reserved_mem_end_phys_addr){
        reserved_mem_end_phys_addr = last_module_phys_addr;
    }
    // printf("reserved_mem_end_phys_addr: %08x ", reserved_mem_end_phys_addr);
    uint32_t reserved_mem_end_page = (reserved_mem_end_phys_addr + 2*PAGE_SIZE - 1) / PAGE_SIZE;
    // printf("reserved end page: %08x ", reserved_mem_end_page);    

    flags =  PAGE_PRESENT | PAGE_WRITE;
    for(i = 0; i < reserved_mem_end_page; i++){
        // printf("imod: %08x %08x %08x ", i%PAGE_TABLE_SIZE, i, reserved_mem_end_page);
        // mappage_phys_to_virt(&kernel_pagedir, i, i, PAGE_PRESENT | PAGE_WRITE, 1);
        pagetables_phys_addr[i/PAGE_TABLE_SIZE].entries[i%PAGE_TABLE_SIZE] = (i * PAGE_SIZE) | flags;
    }

    uint32_t kernel_end_pde = (reserved_mem_end_page + PAGE_TABLE_SIZE - 1)/PAGE_TABLE_SIZE;
    // printf("kpdpa: %08x ptpa: %08x kep: %08x kpde_c: %08x \n", kernel_pagedir_phys_addr, pagetables_phys_addr, reserved_mem_end_page, kernel_end_pde);
    flags =  PAGE_PRESENT | PAGE_WRITE;
    for(i = 0; i < kernel_end_pde ; i++){
        // printf("setting pde: %08x before: %08x val: %08x %08x \n", i, kernel_pagedir_phys_addr->entries[i], (uint32_t)&pagetables_phys_addr[i], ((uint32_t)&pagetables_phys_addr[i]) | PAGE_PRESENT | PAGE_WRITE);
        kernel_pagedir_phys_addr->entries[i] = ((uint32_t)&pagetables_phys_addr[i]) | flags; // map phys addresses to +0xC0000000 virt addr
        kernel_pagedir_phys_addr->entries[i+768] = ((uint32_t)&pagetables_phys_addr[i]) | flags; // map phys addresses to +0xC0000000 virt addr
    }

    last_pagetable_idx = i;


    // mapping high high map area (usually out of actual ram used by vbe etc)
    // fill PTEs
    uint32_t high_mem_start_page = 0xe0000000 / PAGE_SIZE;
    uint32_t high_mem_end_page = 0xffffffff /PAGE_SIZE;
    flags = PAGE_PRESENT | PAGE_WRITE;
    for (uint32_t i= high_mem_start_page; i<high_mem_end_page; i++){
        // printf(" mapping2: %08x %08x %08x %08x", i,i*PAGE_SIZE| flags, high_mem_start_page, high_mem_end_page);
        pagetables_phys_addr[last_pagetable_idx + (i-high_mem_start_page)/PAGE_TABLE_SIZE].entries[i%PAGE_TABLE_SIZE] = (i * PAGE_SIZE) | flags;
        // map_phys_to_virt(&kernel_pagedir, i*PAGE_SIZE, i*PAGE_SIZE, flags, 1);
    }

    // fill PDEs
    uint32_t high_mem_start_pde = 0xe0000000 / (PAGE_SIZE*PAGE_TABLE_SIZE);
    uint32_t high_mem_end_pde = 0xffffffff / (PAGE_SIZE*PAGE_TABLE_SIZE);
    flags = PAGE_PRESENT | PAGE_WRITE;
    for(i = high_mem_start_pde; i <= high_mem_end_pde ; i++){
        // printf(" mapping3: %08x %08x %08x %08x ", i, (uint32_t)&pagetables_phys_addr[i], high_mem_start_pde, high_mem_end_pde);
        kernel_pagedir_phys_addr->entries[i] = ((uint32_t)&pagetables_phys_addr[last_pagetable_idx++]) | flags; // map phys addresses to +0xC0000000 virt addr
    }

    for(i=0; i< reserved_mem_end_page ; i++){
        paging_state.mapped_phys_page_bitmap[i/32] |= (1 << (i % 32));
    }


    // now we try to map addresses here and there that are out of memory range to themselves

    // printf(" mapped_phys_page_bitmap:%08x ", &paging_state.mapped_phys_page_bitmap);
    uint32_t tmp_start_page, tmp_end_page;
    tmp_start_page = 0x09000000 / PAGE_SIZE;
    tmp_end_page = 0x09001000 / PAGE_SIZE;
    for (uint32_t i= tmp_start_page; i<tmp_end_page; i++){
        // printf(" mapping2: %08x %08x %08x %08x", i,i*PAGE_SIZE| flags, high_mem_start_page, high_mem_end_page);
        pagetables_phys_addr[last_pagetable_idx + (i-tmp_start_page)/PAGE_TABLE_SIZE].entries[i%PAGE_TABLE_SIZE] = (i * PAGE_SIZE) | flags;
        // map_phys_to_virt(&kernel_pagedir, i*PAGE_SIZE, i*PAGE_SIZE, flags, 1);
    }
    tmp_start_page /= PAGE_TABLE_SIZE;
    tmp_end_page /= PAGE_TABLE_SIZE;
    for(i = tmp_start_page; i <= tmp_end_page ; i++){
        printf(" mapping3: %08x %08x %08x %08x ",   i, (uint32_t)&pagetables_phys_addr[last_pagetable_idx+1], tmp_start_page, tmp_end_page);
        kernel_pagedir_phys_addr->entries[i] = ((uint32_t)&pagetables_phys_addr[last_pagetable_idx++]) | flags; // map phys addresses to +0xC0000000 virt addr
    }
    
    tmp_start_page = 0xC0000000 / PAGE_SIZE;
    tmp_end_page = 0xC0400000 / PAGE_SIZE;
    for (uint32_t i= tmp_start_page; i<tmp_end_page; i++){
        // printf(" mapping2: %08x %08x %08x %08x", i,i*PAGE_SIZE| flags, high_mem_start_page, high_mem_end_page);
        if( i >= 0xC0005 && i < 0xC0006){
            pagetables_phys_addr[last_pagetable_idx + (i-tmp_start_page)/PAGE_TABLE_SIZE].entries[i%PAGE_TABLE_SIZE] = (i * PAGE_SIZE) | flags;
        }else{
            pagetables_phys_addr[last_pagetable_idx + (i-tmp_start_page)/PAGE_TABLE_SIZE].entries[i%PAGE_TABLE_SIZE] = ( (i-tmp_start_page) * PAGE_SIZE) | flags;
        }
        // map_phys_to_virt(&kernel_pagedir, i*PAGE_SIZE, i*PAGE_SIZE, flags, 1);
    }
    tmp_start_page = 0xC0005000 / PAGE_SIZE;
    tmp_end_page = 0xC0006000 / PAGE_SIZE;
    tmp_start_page /= PAGE_TABLE_SIZE;
    tmp_end_page /= PAGE_TABLE_SIZE;
    for(i = tmp_start_page; i <= tmp_end_page ; i++){
        printf(" mapping4: %08x %08x %08x %08x ", i, (uint32_t)&pagetables_phys_addr[last_pagetable_idx+1], tmp_start_page, tmp_end_page);
        kernel_pagedir_phys_addr->entries[i] = ((uint32_t)&pagetables_phys_addr[last_pagetable_idx++]) | flags; // map phys addresses to +0xC0000000 virt addr
    }
    

    
    
    // last_pagetable_idx = kernel_end_pde;
    enable_paging((page_directory_t*) kernel_pagedir_phys_addr);
    printf(" paging enabled ");

}



void init_paging_stage2(multiboot_info_t* mbi) {
    last_pagetable_idx=0;
    calculate_total_pages(mbi);

    print_mmap_addresses(mbi);
    print_module_info(mbi);
    // while(1);

    // calculate_bitmap_size_and_pagetables();

    mark_initial_used_physical_pages(mbi);
    // while(1);

    // paging_state.total_pagetables = paging_state.total_pages / 1024;
    // if (paging_state.total_pagetables > 1024) paging_state.total_pagetables = 1024;

    // // Set physical start of page tables after kernel
    // paging_state.pagetables_phys_start = (_kernel_end_phys > 0x400000 ? _kernel_end_phys : 0x400000);
    // paging_state.pagetables_phys_start = (paging_state.pagetables_phys_start + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

    // // Map pagetables_virt_start (0xFFC00000) to pagetables_phys_start
    // for (int i = 0; i < 1024; i++) {
    //     pagetables_pagetable[i] = (paging_state.pagetables_phys_start + i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    // }
    // pagedir[1023] = ((uint32_t)pagetables_pagetable - KERNEL_VBASE) | PAGE_PRESENT | PAGE_WRITE;

    // // Initialize page tables at pagetables_virt_start
    // uint32_t* tmp_pagetable;
    // for (int i = 0; i < paging_state.total_pagetables; i++) {
    //     tmp_pagetable = (uint32_t*)(paging_state.pagetables_virt_start + i * PAGE_SIZE);
    //     for (int j = 0; j < 1024; j++) {
    //         tmp_pagetable[j] = ((i * 1024 + j) * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    //     }
    // }

    // // Initialize pagetable bitmap
    // paging_state.used_pagetables = 0;
    // uint32_t pagetables_phys_end = paging_state.pagetables_phys_start + 0x400000;
    // uint32_t first_free_pagetable = (pagetables_phys_end + (1024 * PAGE_SIZE) - 1) / (1024 * PAGE_SIZE);
    // for (int i = 0; i < first_free_pagetable; i++) {
    //     paging_state.pagetable_bitmap[i / 8] |= (1 << (i % 8));
    //     paging_state.used_pagetables++;
    // }

    // // we hold pagedirs of processes inside pagedirs
    // // we make sure they are initilized with 0
    // for(int i=0; i< MAX_PROCESSES; i++){
    //     for(int j=0; j< 1024; j++){
    //         pagedirs[i][j]=0;
    //     }
    // }
}

// static inline uint32_t* get_pagetable_phys_byidx(int pt_idx){
//     return paging_state.pagetables_phys_start + pt_idx * PAGE_SIZE;    
// }

// static inline uint32_t* get_pagedir_phys_byidx(int pd_idx){
//     // return pagedirs - KERNEL_VBASE + pd_idx * PAGE_SIZE; // this should do the same   
//     return (&pagedirs[pd_idx][0]) - KERNEL_VBASE;    
// }


void switch_page_directory(uint32_t* new_pagedir_phys) {
    asm volatile("mov %0, %%cr3" : : "r" (new_pagedir_phys));
}

uint32_t* get_current_page_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    return (uint32_t*)cr3; // Physical address
}

// uint32_t* get_pagedir_virtaddr(uint32_t phys_addr) {
//     if(phys_addr  & 0x3FF) return 0; // not 4k aligned
//     uint32_t min_phys_addr = pagedirs - KERNEL_VBASE;
//     uint32_t max_phys_addr = pagedirs + MAX_PROCESSES - KERNEL_VBASE;
//     if(phys_addr < min_phys_addr || phys_addr>max_phys_addr) return 0;

//     return (uint32_t*) phys_addr + KERNEL_VBASE;
// }

// uint32_t* create_process_pd() {
//     // Check if enough page tables are available (90% cap)
//     if (paging_state.used_pagetables * 10 > paging_state.total_pagetables * 9) {
//         return NULL; // Not enough free page tables for process mappings
//     }

//     // Find free page directory index
//     int pd_idx = get_free_pagedir_idx();
//     if (pd_idx < 0) return NULL; // No free page directories

//     // Get physical and virtual addresses
//     uint32_t* new_pd_phys = get_pagedir_phys_byidx(pd_idx);
//     uint32_t* new_pd = get_pagedir_virtaddr((uint32_t)new_pd_phys);
//     if (!new_pd) return NULL; // Invalid virtual address mapping

//     // Clear page directory
//     for (int i = 0; i < 1024; i++) {
//         new_pd[i] = 0;
//     }

//     // Copy kernel mappings (0xC0000000-0xFFFFFFFF)
//     for (int i = 768; i < 1024; i++) {
//         if (pagedir[i] & PAGE_PRESENT) {
//             new_pd[i] = pagedir[i];
//         }
//     }

//     return new_pd_phys;
// }

// int assign_page_table(uint32_t* page_dir, void* virt_addr, uint32_t flags) {
//     if ((uint32_t)virt_addr & 0x3FFFFF) return ASSIGN_PAGE_NOT_ALIGNED;
//     uint32_t pd_idx = (uint32_t)virt_addr >> 22;
//     if (pd_idx >= 768 && pd_idx < 1023) printf(" pd_idx: %08X vaddr: %08X ", pd_idx, virt_addr);//return ASSIGN_PAGE_KERNEL_SPACE;

//     uint32_t* pd = get_pagedir_virtaddr((uint32_t)page_dir);
//     if (pd[pd_idx] & PAGE_PRESENT) return ASSIGN_PAGE_ALREADY_MAPPED;

//     int pt_idx = get_free_pagetable_idx();
//     if (pt_idx < 0) return ASSIGN_PAGE_NO_TABLES;

//     uint32_t pt_phys = get_pagetable_phys_byidx(pt_idx);
//     pd[pd_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);    

//     uint32_t* current_pd = get_current_page_directory();
//     if (page_dir == current_pd) {
//         asm volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
//     }

//     return ASSIGN_PAGE_OK;
// }

// int get_free_pagedir_idx(){
//     for (int i = 0; i < MAX_PROCESSES; i++) {
//         if (!(paging_state.pagedir_bitmap[i / 8] & (1 << (i % 8)))) {
//             paging_state.pagedir_bitmap[i / 8] |= (1 << (i % 8));
//             paging_state.used_pagedirs++;
//             return i;
//         }
//     }
//     return -1;
// }

// void free_pagedir_idx(int idx) {
//     if (idx < 0 || idx >= MAX_PROCESSES) {
//         printf("Invalid pagedir index: %d\n", idx);
//         return;
//     }
//     if (!(paging_state.pagedir_bitmap[idx / 8] & (1 << (idx % 8)))) {
//         printf("pagedir %d already free\n", idx);
//         return;
//     }

//     paging_state.pagedir_bitmap[idx / 8] &= ~(1 << (idx % 8));
//     paging_state.used_pagedirs--;
// }

// FreePageStatus free_pagedir_by_vaddr(void* vaddr) {
//     uint32_t addr = (uint32_t)vaddr;
//     uint32_t pagedirs_addr = (uint32_t)pagedirs;
    
//     if (addr < pagedirs_addr || addr >= pagedirs_addr + MAX_PROCESSES) {
//         return FREE_PAGE_OUT_OF_RANGE;
//     }
//     if (addr & (PAGE_SIZE - 1)) {
//         return FREE_PAGE_NOT_ALIGNED;
//     }

//     int idx = (addr - pagedirs_addr) / PAGE_SIZE;
//     if (idx >= MAX_PROCESSES) {
//         return FREE_PAGE_EXCEEDS_POOL;
//     }

//     free_pagedir_idx(idx);
//     return FREE_PAGE_OK;
// }

// int get_free_pagetable_idx() {
//     for (int i = 0; i < paging_state.total_pagetables; i++) {
//         if (!(paging_state.pagetable_bitmap[i / 8] & (1 << (i % 8)))) {
//             paging_state.pagetable_bitmap[i / 8] |= (1 << (i % 8));
//             paging_state.used_pagetables++;
//             return i;
//         }
//     }
//     return -1;
// }

// void free_pagetable_idx(int idx) {
//     if (idx < 0 || idx >= paging_state.total_pagetables) {
//         printf("Invalid pagetable index: %d\n", idx);
//         return;
//     }
//     if (!(paging_state.pagetable_bitmap[idx / 8] & (1 << (idx % 8)))) {
//         printf("Pagetable %d already free\n", idx);
//         return;
//     }

//     paging_state.pagetable_bitmap[idx / 8] &= ~(1 << (idx % 8));
//     paging_state.used_pagetables--;
// }

// FreePageStatus free_pagetable_by_vaddr(void* vaddr) {
//     uint32_t addr = (uint32_t)vaddr;
//     if (addr < paging_state.pagetables_virt_start || addr >= 0xFFFFFFFF) {
//         return FREE_PAGE_OUT_OF_RANGE;
//     }
//     if (addr & (PAGE_SIZE - 1)) {
//         return FREE_PAGE_NOT_ALIGNED;
//     }

//     int idx = (addr - paging_state.pagetables_virt_start) / PAGE_SIZE;
//     if (idx >= paging_state.total_pagetables) {
//         return FREE_PAGE_EXCEEDS_POOL;
//     }

//     free_pagetable_idx(idx);
//     return FREE_PAGE_OK;
// }