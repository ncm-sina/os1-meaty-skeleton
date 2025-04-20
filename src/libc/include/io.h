#ifndef _IO_H
#define _IO_H

// #include <libk/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Byte I/O (8-bit) */
inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Word I/O (16-bit) */
inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Long I/O (32-bit) */
inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* String I/O (block transfers) */
inline void insb(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile ("rep insb" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

inline void outsb(uint16_t port, const void *addr, uint32_t count) {
    __asm__ volatile ("rep outsb" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

inline void insw(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile ("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

inline void outsw(uint16_t port, const void *addr, uint32_t count) {
    __asm__ volatile ("rep outsw" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

inline void insl(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile ("rep insl" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

inline void outsl(uint16_t port, const void *addr, uint32_t count) {
    __asm__ volatile ("rep outsl" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

/* I/O delay for synchronization */
inline void io_wait(void) {
    __asm__ volatile ("outb %0, $0x80" : : "a"(0));
}

#ifdef __cplusplus
}
#endif

#endif