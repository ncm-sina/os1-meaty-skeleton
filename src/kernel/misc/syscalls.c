#include <kernel/misc/syscalls.h>
#include <errno.h>

long sys_times(struct tms *buf) {
    return -ENOSYS; // TODO: Implement
}

long sys_uname(struct utsname *buf) {
    return -ENOSYS; // TODO: Implement
}

long sys_getcwd(char *buf, size_t size) {
    return -ENOSYS; // TODO: Implement
}

long sys_fcntl(int fd, int cmd, long arg) {
    return -ENOSYS; // TODO: Implement
}

long sys_writev(int fd, const struct iovec *iov, int iovcnt) {
    return -ENOSYS; // TODO: Implement
}

long sys_nanosleep(const struct timespec *req, struct timespec *rem) {
    return -ENOSYS; // TODO: Implement
}

long sys_gettimeofday(struct timeval *tv, struct timezone *tz) {
    return -ENOSYS; // TODO: Implement
}

long sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    return -ENOSYS; // TODO: Implement
}