#ifndef _SYSCALL_ARCH_H
#define _SYSCALL_ARCH_H
#include <errno.h>

static inline long __syscall0(long n) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

static inline long __syscall1(long n, long a1) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "b"(a1)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

static inline long __syscall2(long n, long a1, long a2) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "b"(a1), "c"(a2)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

static inline long __syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "b"(a1), "c"(a2), "d"(a3)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

static inline long __syscall4(long n, long a1, long a2, long a3, long a4) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

static inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

static inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    __asm__ volatile (
        "push %%ebp\n\t"
        "mov %7, %%ebp\n\t"
        "int $0x80\n\t"
        "pop %%ebp"
        : "=a"(ret)
        : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5), "m"(a6)
        : "memory"
    );
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

#endif