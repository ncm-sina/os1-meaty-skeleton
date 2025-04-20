#include <signal.h>
#include <errno.h>
#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    long ret = __syscall3(SYS_sigaction, (long)signum, (long)act, (long)oldact);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}