#include <unistd.h>
#include <errno.h>
#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

/* File I/O */
int open(const char *pathname, int flags, mode_t mode) {
    long ret = __syscall3(SYS_open, (long)pathname, (long)flags, (long)mode);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
    long ret = __syscall3(SYS_read, (long)fd, (long)buf, (long)count);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
    long ret = __syscall3(SYS_write, (long)fd, (long)buf, (long)count);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int close(int fd) {
    long ret = __syscall1(SYS_close, (long)fd);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

off_t lseek(int fd, off_t offset, int whence) {
    long ret = __syscall3(SYS_lseek, (long)fd, (long)offset, (long)whence);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int access(const char *pathname, int mode) {
    long ret = __syscall2(SYS_access, (long)pathname, (long)mode);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int unlink(const char *pathname) {
    long ret = __syscall1(SYS_unlink, (long)pathname);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int chdir(const char *path) {
    long ret = __syscall1(SYS_chdir, (long)path);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int mkdir(const char *pathname, mode_t mode) {
    long ret = __syscall2(SYS_mkdir, (long)pathname, (long)mode);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int rmdir(const char *pathname) {
    long ret = __syscall1(SYS_rmdir, (long)pathname);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int pipe(int pipefd[2]) {
    long ret = __syscall1(SYS_pipe, (long)pipefd);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int dup(int oldfd) {
    long ret = __syscall1(SYS_dup, (long)oldfd);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int dup2(int oldfd, int newfd) {
    long ret = __syscall2(SYS_dup2, (long)oldfd, (long)newfd);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int ftruncate(int fd, off_t length) {
    long ret = __syscall2(SYS_ftruncate, (long)fd, (long)length);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int fchmod(int fd, mode_t mode) {
    long ret = __syscall2(SYS_fchmod, (long)fd, (long)mode);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int stat(const char *pathname, struct stat *buf) {
    long ret = __syscall2(SYS_stat, (long)pathname, (long)buf);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int lstat(const char *pathname, struct stat *buf) {
    long ret = __syscall2(SYS_lstat, (long)pathname, (long)buf);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int fstat(int fd, struct stat *buf) {
    long ret = __syscall2(SYS_fstat, (long)fd, (long)buf);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    long ret = __syscall3(SYS_writev, (long)fd, (long)iov, (long)iovcnt);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

char *getcwd(char *buf, size_t size) {
    long ret = __syscall2(SYS_getcwd, (long)buf, (long)size);
    if (ret < 0) { errno = -ret; return NULL; }
    return (char *)ret;
}

/* Process management */
void _exit(int status) {
    __syscall1(SYS_exit, (long)status);
    __builtin_unreachable();
}

pid_t fork(void) {
    long ret = __syscall0(SYS_fork);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

pid_t waitpid(pid_t pid, int *status, int options) {
    long ret = __syscall3(SYS_waitpid, (long)pid, (long)status, (long)options);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    long ret = __syscall3(SYS_execve, (long)filename, (long)argv, (long)envp);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}

pid_t getpid(void) {
    return __syscall0(SYS_getpid);
}

pid_t getppid(void) {
    return __syscall0(SYS_getppid);
}

uid_t getuid(void) {
    return __syscall0(SYS_getuid);
}

gid_t getgid(void) {
    return __syscall0(SYS_getgid);
}

uid_t geteuid(void) {
    return __syscall0(SYS_geteuid);
}

gid_t getegid(void) {
    return __syscall0(SYS_getegid);
}

int kill(pid_t pid, int sig) {
    long ret = __syscall2(SYS_kill, (long)pid, (long)sig);
    if (ret < 0) { errno = -ret; return -1; }
    return ret;
}