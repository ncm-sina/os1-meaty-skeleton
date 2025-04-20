#include <kernel/process/syscalls.h>
#include <errno.h>

long sys_exit(int status) {
    return -ENOSYS; // TODO: Implement
}

long sys_fork(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_waitpid(pid_t pid, int *status, int options) {
    return -ENOSYS; // TODO: Implement
}

long sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    return -ENOSYS; // TODO: Implement
}

long sys_getpid(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_getppid(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_setuid(uid_t uid) {
    return -ENOSYS; // TODO: Implement
}

long sys_setgid(gid_t gid) {
    return -ENOSYS; // TODO: Implement
}

long sys_getuid(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_getgid(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_geteuid(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_getegid(void) {
    return -ENOSYS; // TODO: Implement
}

long sys_kill(pid_t pid, int sig) {
    return -ENOSYS; // TODO: Implement
}

long sys_chroot(const char *path) {
    return -ENOSYS; // TODO: Implement
}