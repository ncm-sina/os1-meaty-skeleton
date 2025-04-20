#include <sys/syscall.h>
#include <stdarg.h>

long syscall(long n, ...) {
    va_list ap;
    va_start(ap, n);
    long a1 = va_arg(ap, long);
    long a2 = va_arg(ap, long);
    long a3 = va_arg(ap, long);
    long a4 = va.arg(ap, long);
    long a5 = va.arg(ap, long);
    long a6 = va_arg(ap, long);
    va_end(ap);

    switch (n) {
        case SYS_mmap:
            return __syscall6(n, a1, a2, a3, a4, a5, a6);
        case SYS_read:
        case SYS_write:
        case SYS_open:
        case SYS_waitpid:
        case SYS_execve:
        case SYS_lseek:
        case SYS_fcntl:
        case SYS_writev:
        case SYS_sigaction:
            return __syscall3(n, a1, a2, a3);
        case SYS_unlink:
        case SYS_chdir:
        case SYS_access:
        case SYS_mkdir:
        case SYS_rmdir:
        case SYS_getcwd:
        case SYS_ftruncate:
        case SYS_fchmod:
        case SYS_stat:
        case SYS_lstat:
        case SYS_fstat:
        case SYS_kill:
        case SYS_nanosleep:
        case SYS_gettimeofday:
        case SYS_pipe:
        case SYS_dup2:
            return __syscall2(n, a1, a2);
        case SYS_exit:
        case SYS_close:
        case SYS_brk:
        case SYS_munmap:
        case SYS_uname:
        case SYS_setuid:
        case SYS_setgid:
        case SYS_chroot:
        case SYS_dup:
            return __syscall1(n, a1);
        case SYS_fork:
        case SYS_getpid:
        case SYS_getppid:
        case SYS_getuid:
        case SYS_getgid:
        case SYS_times:
        case SYS_geteuid:
        case SYS_getegid:
            return __syscall0(n);
        default:
            errno = ENOSYS;
            return -1;
    }
}