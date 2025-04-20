#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

// #include <sys/cdefs.h>

// #include <stddef.h>

#include <sys/types.h>

#define PROT_NONE  0
#define PROT_READ  1
#define PROT_WRITE 2
#define PROT_EXEC  4

#define MAP_SHARED  1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20

#ifdef __cplusplus
extern "C" {
#endif

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);
void *brk(void *addr);

#ifdef __cplusplus
}
#endif

#endif