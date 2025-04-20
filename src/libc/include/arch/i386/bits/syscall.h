#ifndef _BITS_SYSCALL_H
#define _BITS_SYSCALL_H

#define SYS_exit           1
#define SYS_fork           2
#define SYS_read           3
#define SYS_write          4
#define SYS_open           5
#define SYS_close          6
#define SYS_waitpid        7
#define SYS_unlink        12
#define SYS_execve        13
#define SYS_chdir         14
#define SYS_lseek         19
#define SYS_getpid        20
#define SYS_setuid        23
#define SYS_access        33
#define SYS_mkdir         39
#define SYS_rmdir         40
#define SYS_dup           41
#define SYS_pipe          42
#define SYS_times         43
#define SYS_brk           45
#define SYS_setgid        46
#define SYS_getppid       47
#define SYS_getuid        54
#define SYS_getgid        56
#define SYS_chroot        61
#define SYS_dup2          63
#define SYS_kill          67
#define SYS_getcwd        83
#define SYS_mmap          90
#define SYS_munmap        91
#define SYS_ftruncate     93
#define SYS_fchmod        94
#define SYS_stat          99
#define SYS_lstat        100
#define SYS_fstat        101
#define SYS_uname        122
#define SYS_fcntl        125
#define SYS_geteuid      132
#define SYS_getegid      133
#define SYS_writev       146
#define SYS_nanosleep    162
#define SYS_gettimeofday 168
#define SYS_sigaction    172
#define SYS_mmap2        192

#endif