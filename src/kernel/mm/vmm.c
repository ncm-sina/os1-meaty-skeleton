#include <kernel/mm/vmm.h>
// #include <kernel/mm/paging.h>
// #include <kernel/mm.h>
#include <kernel/process.h>
#include <errno.h>

/* Current process (placeholder) */
extern struct process *current_process;

/* Allocate a new virtual memory area */
static struct vm_area *alloc_vma(void *start, size_t length, int prot, int flags, int fd, off_t offset) {
    // struct vm_area *vma = kmalloc(sizeof(struct vm_area));
    // if (!vma) return NULL;
    // vma->vma_start = start;
    // vma->vma_length = length;
    // vma->vma_prot = prot;
    // vma->vma_flags = flags;
    // vma->vma_fd = fd;
    // vma->vma_offset = offset;
    // vma->next = NULL;
    // return vma;
}

/* Map physical pages to virtual address */
static int map_pages(void *vaddr, size_t length, int prot) {
    // for (size_t offset = 0; offset < length; offset += PAGE_SIZE) {
    //     void *phys = alloc_page();
    //     if (!phys) return -ENOMEM;
    //     int flags = (prot & PROT_WRITE) ? PAGE_WRITE : 0;
    //     if (vmm_map_page(phys, vaddr + offset, flags) < 0) {
    //         free_page(phys);
    //         return -ENOMEM;
    //     }
    // }
    // return 0;
}

void *vm_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    // if (!current_process || length == 0 || (flags & ~MAP_PRIVATE & ~MAP_ANONYMOUS & ~MAP_FIXED)) {
    //     return MAP_FAILED;
    // }

    // /* Round length to page size */
    // length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    // /* Find a free virtual address */
    // void *vaddr = addr;
    // if (!(flags & MAP_FIXED)) {
    //     vaddr = (void *)0x40000000; /* Simple allocation above user code */
    //     /* TODO: Check for collisions with existing VMAs */
    // }

    // /* Allocate VMA */
    // struct vm_area *vma = alloc_vma(vaddr, length, prot, flags, fd, offset);
    // if (!vma) return MAP_FAILED;

    // /* Map pages (anonymous memory only for now) */
    // if (flags & MAP_ANONYMOUS) {
    //     if (map_pages(vaddr, length, prot) < 0) {
    //         kfree(vma);
    //         return MAP_FAILED;
    //     }
    // } else {
    //     /* TODO: File-backed mappings */
    //     kfree(vma);
    //     return MAP_FAILED;
    // }

    // /* Add VMA to process */
    // vma->next = current_process->vma_list;
    // current_process->vma_list = vma;

    // return vaddr;
}

int vm_munmap(void *addr, size_t length) {
    // if (!current_process || !addr || length == 0) {
    //     return -EINVAL;
    // }

    // /* Round length to page size */
    // length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    // /* Find and remove VMA */
    // struct vm_area **prev = &current_process->vma_list;
    // struct vm_area *vma = *prev;
    // while (vma) {
    //     if (vma->vma_start == addr && vma->vma_length == length) {
    //         *prev = vma->next;
    //         /* Unmap pages */
    //         for (size_t offset = 0; offset < length; offset += PAGE_SIZE) {
    //             vmm_unmap_page(vaddr + offset);
    //         }
    //         kfree(vma);
    //         return 0;
    //     }
    //     prev = &vma->next;
    //     vma = *prev;
    // }

    // return -EINVAL;
}

void *heap_adjust(void *addr, size_t size) {
    // if (!current_process) {
    //     return NULL;
    // }

    // void *old_break = current_process->heap_end;
    // void *new_break = addr ? addr : old_break + size;

    // /* Align to page size */
    // new_break = (void *)(((uint32_t)new_break + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

    // if (new_break < current_process->heap_start) {
    //     return NULL; /* Cannot shrink below heap start */
    // }

    // /* Expand or shrink heap */
    // if (new_break > old_break) {
    //     size_t length = new_break - old_break;
    //     if (map_pages(old_break, length, PROT_READ | PROT_WRITE) < 0) {
    //         return NULL;
    //     }
    // } else if (new_break < old_break) {
    //     size_t length = old_break - new_break;
    //     for (size_t offset = 0; offset < length; offset += PAGE_SIZE) {
    //         vmm_unmap_page(new_break + offset);
    //     }
    // }

    // current_process->heap_end = new_break;
    // return new_break;
}
