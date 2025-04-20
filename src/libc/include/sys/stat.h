#ifndef _STAT_H
#define _STAT_H 1

// #include <sys/cdefs.h>

// #include <stddef.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int mkdir(const char *path, mode_t mode);
int chmod(const char *path, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
