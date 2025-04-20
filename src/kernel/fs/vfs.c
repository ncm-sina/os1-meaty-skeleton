#include <kernel/fs/vfs.h>
#include <errno.h>

int vfs_open(const char *pathname, int flags, mode_t mode) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_read(int fd, void *buf, size_t count) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_write(int fd, const void *buf, size_t count) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_unlink(const char *pathname) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_close(int fd) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_lseek(int fd, off_t offset, unsigned int origin) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_stat(const char *pathname, struct stat *statbuf) {
    return -ENOSYS; /* TODO: Implement */
}

int vfs_access(const char *pathname, int mode) {
    return -ENOSYS; /* TODO: Implement */
}