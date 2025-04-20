#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

int open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    long ret = __syscall3(SYS_open, (long)path, (long)flags, (long)mode);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int fcntl(int fd, int cmd, ...) {
    long arg = 0;
    if (cmd == F_DUPFD || cmd == F_SETFD || cmd == F_SETFL) {
        va_list args;
        va_start(args, cmd);
        arg = va_arg(args, long);
        va_end(args);
    }
    long ret = __syscall3(SYS_fcntl, (long)fd, (long)cmd, (long)arg);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}