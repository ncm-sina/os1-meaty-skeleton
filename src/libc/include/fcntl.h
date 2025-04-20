#ifndef _FCNTL_H
#define _FCNTL_H

// #include <sys/cdefs.h>

// #include <stddef.h>

#include <sys/types.h>

#define O_RDONLY  0
#define O_WRONLY  1
#define O_RDWR    2
#define O_CREAT   0100
#define O_EXCL    0200
#define O_TRUNC   01000
#define O_APPEND  02000

#define F_DUPFD   0
#define F_GETFD   1
#define F_SETFD   2
#define F_GETFL   3
#define F_SETFL   4

#ifdef __cplusplus
extern "C" {
#endif


int open(const char *path, int flags, ... /* mode_t mode */);
int fcntl(int fd, int cmd, ... /* long arg */);

// int creat(const char *path, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
