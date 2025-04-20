#ifndef _KERNEL_MM_VMM_H
#define _KERNEL_MM_VMM_H

#include <sys/types.h>
#include <errno.h>

/* Memory protection flags */
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

/* Mapping flags */
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FIXED     0x10

/* Virtual memory area */
struct vm_area {
    void *vma_start;        /* Start address */
    size_t vma_length;      /* Length in bytes */
    int vma_prot;           /* Protection flags (PROT_*) */
    int vma_flags;          /* Mapping flags (MAP_*) */
    int vma_fd;             /* File descriptor (or -1 for anonymous) */
    off_t vma_offset;       /* File offset */
    struct vm_area *next;   /* Next VMA in process */
};

/* Process structure (simplified) */
struct process {
    struct vm_area *vma_list; /* List of virtual memory areas */
    void *heap_start;         /* Start of heap */
    void *heap_end;           /* Current heap break */
    /* ... other fields ... */
};

/* Virtual memory functions */
void *vm_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int vm_munmap(void *addr, size_t length);
void *heap_adjust(void *addr, size_t size);

#endif