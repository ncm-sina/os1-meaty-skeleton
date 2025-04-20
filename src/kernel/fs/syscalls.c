#include <kernel/fs/syscalls.h>
#include <kernel/fs/vfs.h>
#include <errno.h>


// long sys_open(const char *filename, int flags, mode_t mode) {
//     if (!filename) return -EINVAL;
//     struct file *file;
//     int ret = vfs_open(filename, flags, mode, &file);
//     if (ret < 0) return ret;
//     for (int fd = 0; fd < 1024; fd++) {
//         if (!fd_table[fd]) {
//             fd_table[fd] = file;
//             return fd;
//         }
//     }
//     vfs_close(file);
//     return -EMFILE;
// }

// long sys_read(int fd, char *buf, size_t count) {
//     if (fd < 0 || fd >= 1024 || !fd_table[fd] || !buf) return -EBADF;
//     size_t bytes;
//     int ret = vfs_read(fd_table[fd], buf, count, &bytes);
//     if (ret < 0) return ret;
//     return bytes;
// }

// long sys_write(int fd, const char *buf, size_t count) {
//     if (fd < 0 || fd >= 1024 || !fd_table[fd] || !buf) return -EBADF;
//     size_t bytes;
//     int ret = vfs_write(fd_table[fd], buf, count, &bytes);
//     if (ret < 0) return ret;
//     return bytes;
// }

// long sys_close(int fd) {
//     if (fd < 0 || fd >= 1024 || !fd_table[fd]) return -EBADF;
//     int ret = vfs_close(fd_table[fd]);
//     if (ret < 0) return ret;
//     fd_table[fd] = NULL;
//     return 0;
// }

// long sys_lseek(int fd, off_t offset, unsigned int origin) {
//     if (fd < 0 || fd >= 1024 || !fd_table[fd]) return -EBADF;
//     off_t new_offset;
//     int ret = vfs_lseek(fd_table[fd], offset, origin, &new_offset);
//     if (ret < 0) return ret;
//     return new_offset;
// }

// long sys_access(const char *pathname, int mode) {
//     if (!pathname) return -EINVAL;
//     int ret = vfs_access(pathname, mode);
//     if (ret < 0) return ret;
//     return 0;
// }

// long sys_unlink(const char *pathname) {
//     return -ENOSYS; // TODO: Implement
// }

long sys_open(const char *pathname, int flags, mode_t mode) {
    return vfs_open(pathname, flags, mode);
}

long sys_read(int fd, void *buf, size_t count) {
    return vfs_read(fd, buf, count);
}

long sys_write(int fd, const void *buf, size_t count) {
    return vfs_write(fd, buf, count);
}

long sys_unlink(const char *pathname) {
    return vfs_unlink(pathname);
}

long sys_close(int fd) {
    return vfs_close(fd);
}

long sys_lseek(int fd, off_t offset, unsigned int origin) {
    return vfs_lseek(fd, offset, origin);
}

long sys_access(const char *pathname, int mode) {
    return vfs_access(pathname, mode);
}




long sys_chdir(const char *path) {
    return -ENOSYS; // TODO: Implement
}

long sys_mkdir(const char *pathname, mode_t mode) {
    return -ENOSYS; // TODO: Implement
}

long sys_rmdir(const char *pathname) {
    return -ENOSYS; // TODO: Implement
}

long sys_pipe(int pipefd[2]) {
    return -ENOSYS; // TODO: Implement
}

long sys_dup(int oldfd) {
    return -ENOSYS; // TODO: Implement
}

long sys_dup2(int oldfd, int newfd) {
    return -ENOSYS; // TODO: Implement
}

long sys_ftruncate(int fd, off_t length) {
    return -ENOSYS; // TODO: Implement
}

long sys_fchmod(int fd, mode_t mode) {
    return -ENOSYS; // TODO: Implement
}

long sys_stat(const char *pathname, struct stat *buf) {
    return -ENOSYS; // TODO: Implement
}

long sys_lstat(const char *pathname, struct stat *buf) {
    return -ENOSYS; // TODO: Implement
}

long sys_fstat(int fd, struct stat *buf) {
    // if (fd < 0 || fd >= 1024 || !fd_table[fd] || !buf) return -EBADF;
    // int ret = vfs_stat(fd_table[fd], buf);
    // if (ret < 0) return ret;
    return 0;
}