#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <kernel/multiboot.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_VBASE 0xC0000000
#define PAGETABLES_MAXSIZE 1280
#define PAGE_TABLE_SIZE 1024
#define PAGE_DIR_SIZE 1024


#ifndef MAX_PROCESSES
    #define MAX_PROCESSES 256
#endif

#define PAGETABLES_VIRT_START 0xFFC00000
#define PAGETABLE_BITMAP_SIZE (128)
#define PAGEDIR_BITMAP_SIZE (MAX_PROCESSES / 8)

// Paging flags for 32-bit x86 (i386) page table/directory entries
// Each flag corresponds to a bit in the 12-bit flag field (bits 0–11)

// Bit 0: Present (P)
// Indicates whether the page/page table is present in memory.
// 1 = Present (accessible), 0 = Not present (causes page fault).
#define PAGE_PRESENT  (1 << 0)

// Bit 1: Read/Write (R/W)
// Controls write access to the page.
// 1 = Read/Write, 0 = Read-only (writes cause page fault).
#define PAGE_WRITE    (1 << 1)

// Bit 2: User/Supervisor (U/S)
// Controls access based on privilege level (CPL).
// 1 = User (CPL 0–3, user and kernel), 0 = Supervisor (CPL 0–2, kernel only).
#define PAGE_USER     (1 << 2)

// Bit 3: Page-Level Write-Through (PWT)
// Controls caching policy for the page.
// 1 = Write-through (writes go to cache and memory), 0 = Write-back (writes stay in cache).
#define PAGE_PWT      (1 << 3)

// Bit 4: Page-Level Cache Disable (PCD)
// Enables or disables caching for the page.
// 1 = Cache disabled (direct memory access), 0 = Cache enabled.
#define PAGE_PCD      (1 << 4)

// Bit 5: Accessed (A)
// Indicates if the page has been accessed (read or written).
// 1 = Accessed (set by CPU), 0 = Not accessed (cleared by software).
#define PAGE_ACCESSED (1 << 5)

// Bit 6: Dirty (D)
// Indicates if the page has been written to (PTE only, ignored in PDE).
// 1 = Dirty (set by CPU on write), 0 = Not dirty (cleared by software).
#define PAGE_DIRTY    (1 << 6)

// Bit 7: Page Attribute Table (PAT) or Page Size (PS)
// Context-dependent:
// - PTE: Selects PAT entry for caching (if supported).
// - PDE: 1 = 4MB page (if CR4.PSE=1), 0 = 4KB page table.
#define PAGE_PAT      (1 << 7)  // Or PAGE_PS for PDEs

// Bit 8: Global (G)
// Marks the page as global (TLB not flushed on CR3 change, if CR4.PGE=1).
// 1 = Global (remains in TLB), 0 = Non-global (flushed).
#define PAGE_GLOBAL   (1 << 8)

// Bits 9–11: Available (AVL)
// Reserved for software use, ignored by CPU.
// Used for OS-specific purposes (e.g., reference counting, page state).
#define PAGE_AVL1     (1 << 9)  // Available bit 1
#define PAGE_AVL2     (1 << 10) // Available bit 2
#define PAGE_AVL3     (1 << 11) // Available bit 3

#define M_PHYS_P_BITMAP_MAX_SIZE 32768

// extern uint32_t pagetables_pagetable[1024];
extern void* _kernel_start_virt;
extern void* _kernel_start_phys;
extern void* _kernel_end_virt;
extern void* _kernel_end_phys;

typedef struct {
    uint8_t pagetable_bitmap[PAGETABLE_BITMAP_SIZE];
    uint8_t pagedir_bitmap[PAGEDIR_BITMAP_SIZE];
    int32_t total_pages;
    uint32_t max_addr;
    int32_t total_pagetables;
    uint32_t mapped_phys_page_bitmap[M_PHYS_P_BITMAP_MAX_SIZE];
    uint32_t mapped_phys_page_bitmap_size;
    int32_t used_pagetables;
    int32_t used_pagedirs;
    uint32_t pagetables_phys_start;
    uint32_t pagetables_virt_start;
} PagingState;

// extern PagingState paging_state;

typedef struct {
    uint32_t entries[PAGE_TABLE_SIZE];
} page_table_t __attribute__((aligned(PAGE_SIZE)));

typedef struct {
    uint32_t entries[PAGE_DIR_SIZE];
} page_directory_t __attribute__((aligned(PAGE_SIZE)));


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

extern page_directory_t kernel_pagedir;
extern page_table_t pagetables[PAGETABLES_MAXSIZE];
extern page_directory_t pagedirs;

extern uint32_t last_pagetable_idx;


// Function declarations
void enable_paging(page_directory_t* kernel_pagedir);
void init_paging_stage1(multiboot_info_t* mbi);
int init_paging_stage2(multiboot_info_t* mbi);
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