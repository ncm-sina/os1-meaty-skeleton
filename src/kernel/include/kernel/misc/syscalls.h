#ifndef _KERNEL_MISC_SYSCALLS_H
#define _KERNEL_MISC_SYSCALLS_H

#include <sys/types.h>
#include <kernel/time.h>
#include <signal.h>

long sys_times(struct tms *buf);
long sys_uname(struct utsname *buf);
long sys_getcwd(char *buf, size_t size);
long sys_fcntl(int fd, int cmd, long arg);
long sys_writev(int fd, const struct iovec *iov, int iovcnt);
long sys_nanosleep(const struct timespec *req, struct timespec *rem);
long sys_gettimeofday(struct timeval *tv, struct timezone *tz);
long sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

#endif