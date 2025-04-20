// #include <kernel/arch/i386/idt.h>
#include <arch/i386/bits/syscall.h>
#include <kernel/syscalls.h>

#include <errno.h>


// // Forward declarations for syscall implementations
// long sys_exit(int status);
// long sys_fork(void);
// long sys_read(int fd, char *buf, size_t count);
// long sys_write(int fd, const char *buf, size_t count);
// long sys_open(const char *filename, int flags, int mode);
// long sys_close(int fd);
// long sys_waitpid(pid_t pid, int *status, int options);
// long sys_unlink(const char *pathname);
// long sys_execve(const char *filename, char *const argv[], char *const envp[]);
// long sys_chdir(const char *path);
// long sys_lseek(int fd, off_t offset, unsigned int origin);
// long sys_getpid(void);
// long sys_setuid(uid_t uid);
// long sys_access(const char *pathname, int mode);
// long sys_mkdir(const char *pathname, mode_t mode);
// long sys_rmdir(const char *pathname);
// long sys_dup(int oldfd);
// long sys_pipe(int pipefd[2]);
// long sys_times(struct tms *buf);
// long sys_brk(unsigned long brk);
// long sys_setgid(gid_t gid);
// long sys_getppid(void);
// long sys_getuid(void);
// long sys_getgid(void);
// long sys_chroot(const char *path);
// long sys_dup2(int oldfd, int newfd);
// long sys_kill(pid_t pid, int sig);
// long sys_getcwd(char *buf, size_t size);
// long sys_mmap(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long off);
// long sys_munmap(unsigned long addr, size_t len);
// long sys_ftruncate(int fd, off_t length);
// long sys_fchmod(int fd, mode_t mode);
// long sys_stat(const char *pathname, struct stat *buf);
// long sys_lstat(const char *pathname, struct stat *buf);
// long sys_fstat(int fd, struct stat *buf);
// long sys_uname(struct utsname *buf);
// long sys_fcntl(int fd, int cmd, long arg);
// long sys_geteuid(void);
// long sys_getegid(void);
// long sys_writev(int fd, const struct iovec *iov, int iovcnt);
// long sys_nanosleep(const struct timespec *req, struct timespec *rem);
// long sys_gettimeofday(struct timeval *tv, struct timezone *tz);
// long sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
// long sys_mmap2(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long pgoff);

// Kernel-side dispatcher
long syscall_dispatch(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    switch (n) {
        case SYS_exit:          return sys_exit(a1);
        case SYS_fork:          return sys_fork();
        case SYS_read:          return sys_read(a1, (char *)a2, a3);
        case SYS_write:         return sys_write(a1, (const char *)a2, a3);
        case SYS_open:          return sys_open((const char *)a1, a2, a3);
        case SYS_close:         return sys_close(a1);
        case SYS_waitpid:       return sys_waitpid(a1, (int *)a2, a3);
        case SYS_unlink:        return sys_unlink((const char *)a1);
        case SYS_execve:        return sys_execve((const char *)a1, (char *const *)a2, (char *const *)a3);
        case SYS_chdir:         return sys_chdir((const char *)a1);
        case SYS_lseek:         return sys_lseek(a1, a2, a3);
        case SYS_getpid:        return sys_getpid();
        case SYS_setuid:        return sys_setuid(a1);
        case SYS_access:        return sys_access((const char *)a1, a2);
        case SYS_mkdir:         return sys_mkdir((const char *)a1, a2);
        case SYS_rmdir:         return sys_rmdir((const char *)a1);
        case SYS_dup:           return sys_dup(a1);
        case SYS_pipe:          return sys_pipe((int *)a1);
        case SYS_times:         return sys_times((struct tms *)a1);
        case SYS_brk:           return sys_brk(a1);
        case SYS_setgid:        return sys_setgid(a1);
        case SYS_getppid:       return sys_getppid();
        case SYS_getuid:        return sys_getuid();
        case SYS_getgid:        return sys_getgid();
        case SYS_chroot:        return sys_chroot((const char *)a1);
        case SYS_dup2:          return sys_dup2(a1, a2);
        case SYS_kill:          return sys_kill(a1, a2);
        case SYS_getcwd:        return sys_getcwd((char *)a1, a2);
        case SYS_mmap:          return sys_mmap(a1, a2, a3, a4, a5, a6);
        case SYS_munmap:        return sys_munmap(a1, a2);
        case SYS_ftruncate:     return sys_ftruncate(a1, a2);
        case SYS_fchmod:        return sys_fchmod(a1, a2);
        case SYS_stat:          return sys_stat((const char *)a1, (struct stat *)a2);
        case SYS_lstat:         return sys_lstat((const char *)a1, (struct stat *)a2);
        case SYS_fstat:         return sys_fstat(a1, (struct stat *)a2);
        case SYS_uname:         return sys_uname((struct utsname *)a1);
        case SYS_fcntl:         return sys_fcntl(a1, a2, a3);
        case SYS_geteuid:       return sys_geteuid();
        case SYS_getegid:       return sys_getegid();
        case SYS_writev:        return sys_writev(a1, (const struct iovec *)a2, a3);
        case SYS_nanosleep:     return sys_nanosleep((const struct timespec *)a1, (struct timespec *)a2);
        case SYS_gettimeofday:  return sys_gettimeofday((struct timeval *)a1, (struct timezone *)a2);
        case SYS_sigaction:     return sys_sigaction(a1, (const struct sigaction *)a2, (struct sigaction *)a3);
        case SYS_mmap2:         return sys_mmap2(a1, a2, a3, a4, a5, a6);
        default:                return -ENOSYS;
    }
}

// ISR for int 0x80
// void syscall_handler(struct regs *r) {
//     if (!r) return; // Safety check
//     r->eax = syscall_dispatch(r->eax, r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp);
// }

// // Initialize syscall ISR
// void init_syscalls() {
//     idt_set_gate(128, (unsigned)syscall_handler, 0x08, 0x8E | 0x60); // DPL=3 for user access
// }