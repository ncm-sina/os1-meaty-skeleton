#ifndef _KERNEL_PROCESS_SYSCALLS_H
#define _KERNEL_PROCESS_SYSCALLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/process.h>
#include <sys/types.h>

long sys_exit(int status);
long sys_fork(void);
long sys_waitpid(pid_t pid, int *status, int options);
long sys_execve(const char *filename, char *const argv[], char *const envp[]);
long sys_getpid(void);
long sys_getppid(void);
long sys_setuid(uid_t uid);
long sys_setgid(gid_t gid);
long sys_getuid(void);
long sys_getgid(void);
long sys_geteuid(void);
long sys_getegid(void);
long sys_kill(pid_t pid, int sig);
long sys_chroot(const char *path);

#endif