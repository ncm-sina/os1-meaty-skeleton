#include <kernel/mm/syscalls.h>
#include <kernel/mm/kmalloc.h>
#include <sys\mman.h>
#include <errno.h>


long sys_mmap(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long off) {
    if (len == 0) return -EINVAL;
    struct file *file = (flags & MAP_ANONYMOUS) ? NULL : fd_table[fd];
    if (!(flags & MAP_ANONYMOUS) && (fd < 0 || fd >= 1024 || !file)) return -EBADF;
    unsigned long result;
    int ret = vm_mmap(addr, len, prot, flags, file, off, &result);
    if (ret < 0) return ret;
    return result;
}

long sys_munmap(unsigned long addr, size_t len) {
    if (len == 0) return -EINVAL;
    int ret = vm_munmap(addr, len);
    if (ret < 0) return ret;
    return 0;
}

long sys_brk(unsigned long brk) {
    unsigned long new_brk;
    int ret = heap_adjust(brk, &new_brk);
    if (ret < 0) return ret;
    return new_brk;
}

long sys_mmap2(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long pgoff) {
    return -ENOSYS; // TODO: Implement
}