#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <arch/i386/bits/syscall.h>

// Generic syscall interface
long syscall(long n, ...);

// Inline syscall wrappers
#include <arch/i386/syscall_arch.h>

#endif