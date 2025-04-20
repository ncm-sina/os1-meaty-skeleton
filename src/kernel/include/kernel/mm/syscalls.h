#ifndef _KERNEL_MM_SYSCALLS_H
#define _KERNEL_MM_SYSCALLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// FD table (simplified)
#define MAX_FDS 1024
static struct file *fd_table[MAX_FDS];

long sys_mmap(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long off);
long sys_munmap(unsigned long addr, size_t len);
long sys_brk(unsigned long brk);
long sys_mmap2(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long pgoff);

#endif