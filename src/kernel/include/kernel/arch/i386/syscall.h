#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <kernel/fs/syscalls.h>
#include <kernel/mm/syscalls.h>
#include <kernel/process/syscalls.h>
#include <kernel/misc/syscalls.h>
#include <kernel/arch/i386/syscall.h>

extern void syscall_handler(void);

long syscall_dispatch(long n, long a1, long a2, long a3, long a4, long a5, long a6);
// void syscall_handler(struct regs *r);
// void init_syscalls(void);

#endif