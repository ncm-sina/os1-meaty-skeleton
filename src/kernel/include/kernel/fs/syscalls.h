#ifndef _KERNEL_FS_SYSCALLS_H
#define _KERNEL_FS_SYSCALLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <kernel/fs/vfs.h>

// FD table (simplified)
#define MAX_FDS 1024
static struct file *fd_table[MAX_FDS];

long sys_open(const char *filename, int flags, mode_t mode);
long sys_read(int fd, void *buf, size_t count);
long sys_write(int fd, const void *buf, size_t count);
long sys_close(int fd);
long sys_lseek(int fd, off_t offset, unsigned int origin);
long sys_access(const char *pathname, int mode);
long sys_unlink(const char *pathname);
long sys_chdir(const char *path);
long sys_mkdir(const char *pathname, mode_t mode);
long sys_rmdir(const char *pathname);
long sys_pipe(int pipefd[2]);
long sys_dup(int oldfd);
long sys_dup2(int oldfd, int newfd);
long sys_ftruncate(int fd, off_t length);
long sys_fchmod(int fd, mode_t mode);
long sys_stat(const char *pathname, struct stat *buf);
long sys_lstat(const char *pathname, struct stat *buf);
long sys_fstat(int fd, struct stat *buf);

#endif