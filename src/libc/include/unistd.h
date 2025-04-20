#ifndef _UNISTD_H
#define _UNISTD_H

// #include <sys/cdefs.h>

// #include <stddef.h>

#include <sys/types.h>
#include <sys/syscall.h>
    
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#ifdef __cplusplus
extern "C" {
#endif

/* File I/O */
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);
int access(const char *pathname, int mode);
int unlink(const char *pathname);
int chdir(const char *path);
int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);
int pipe(int pipefd[2]);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int ftruncate(int fd, off_t length);
int fchmod(int fd, mode_t mode);
int stat(const char *pathname, struct stat *buf);
int lstat(const char *pathname, struct stat *buf);
int fstat(int fd, struct stat *buf);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
char *getcwd(char *buf, size_t size);

/* Process management */
void _exit(int status);
pid_t fork(void);
pid_t waitpid(pid_t pid, int *status, int options);
int execve(const char *filename, char *const argv[], char *const envp[]);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
gid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
int kill(pid_t pid, int sig);


#ifdef __cplusplus
}
#endif

#endif
