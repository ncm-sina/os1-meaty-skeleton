#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <sys/types.h>
#include <kernel/time.h>
#include <errno.h>

/* File open flags (matches POSIX) */
#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_CREAT   0x0100
#define O_EXCL    0x0200
#define O_TRUNC   0x0400
#define O_APPEND  0x0800

/* VFS functions */
int vfs_open(const char *pathname, int flags, mode_t mode);
int vfs_read(int fd, void *buf, size_t count);
int vfs_write(int fd, const void *buf, size_t count);
int vfs_unlink(const char *pathname);
int vfs_close(int fd);
int vfs_lseek(int fd, off_t offset, unsigned int origin);
int vfs_stat(const char *pathname, struct stat *statbuf);
int vfs_access(const char *pathname, int mode);

#endif