#include <sys/mman.h>
#include <errno.h>
#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    long ret = __syscall6(SYS_mmap, (long)addr, (long)len, (long)prot, (long)flags, (long)fd, (long)off);
    if (ret < 0) { errno = -ret; return (void *)-1; }
    return (void *)ret;
}

int munmap(void *addr, size_t len) {
    long ret = __syscall2(SYS_munmap, (long)addr, (long)len);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

void *brk(void *addr) {
    long ret = __syscall1(SYS_brk, (long)addr);
    if (ret < 0) { errno = -ret; return (void *)-1; }
    return (void *)ret;
}