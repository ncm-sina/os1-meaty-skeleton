#include <errno.h>
#include <arch/i386/bits/syscall.h>

// // VFS stubs (your OS provides these)
// struct file;
// struct inode;
// struct stat;

// int vfs_open(const char *path, int flags, int mode, struct file **file);
// int vfs_read(struct file *file, char *buf, size_t count, size_t *bytes);
// int vfs_write(struct file *file, const char *buf, size_t count, size_t *bytes);
// int vfs_close(struct file *file);
// int vfs_lseek(struct file *file, off_t offset, unsigned int origin, off_t *new_offset);
// int vfs_stat(struct file *file, struct stat *statbuf);
// int vfs_access(const char *path, int mode);

// FD table (simplified)
#define MAX_FDS 1024
static struct file *fd_table[MAX_FDS];

// long sys_open(const char *filename, int flags, int mode) {
//     if (!filename) return -EINVAL;
//     struct file *file;
//     int ret = vfs_open(filename, flags, mode, &file);
//     if (ret < 0) return ret;

//     // Find free FD
//     int fd;
//     for (fd = 0; fd < MAX_FDS; fd++) {
//         if (!fd_table[fd]) {
//             fd_table[fd] = file;
//             return fd;
//         }
//     }
//     vfs_close(file);
//     return -EMFILE;
// }

// long sys_read(int fd, char *buf, size_t count) {
//     if (fd < 0 || fd >= MAX_FDS || !fd_table[fd] || !buf) return -EBADF;
//     size_t bytes;
//     int ret = vfs_read(fd_table[fd], buf, count, &bytes);
//     if (ret < 0) return ret;
//     return bytes;
// }

// long sys_write(int fd, const char *buf, size_t count) {
//     if (fd < 0 || fd >= MAX_FDS || !fd_table[fd] || !buf) return -EBADF;
//     size_t bytes;
//     int ret = vfs_write(fd_table[fd], buf, count, &bytes);
//     if (ret < 0) return ret;
//     return bytes;
// }

// long sys_close(int fd) {
//     if (fd < 0 || fd >= MAX_FDS || !fd_table[fd]) return -EBADF;
//     int ret = vfs_close(fd_table[fd]);
//     if (ret < 0) return ret;
//     fd_table[fd] = NULL;
//     return 0;
// }

// long sys_lseek(int fd, off_t offset, unsigned int origin) {
//     if (fd < 0 || fd >= MAX_FDS || !fd_table[fd]) return -EBADF;
//     off_t new_offset;
//     int ret = vfs_lseek(fd_table[fd], offset, origin, &new_offset);
//     if (ret < 0) return ret;
//     return new_offset;
// }

// long sys_fstat(int fd, struct stat *statbuf) {
//     if (fd < 0 || fd >= MAX_FDS || !fd_table[fd] || !statbuf) return -EBADF;
//     int ret = vfs_stat(fd_table[fd], statbuf);
//     if (ret < 0) return ret;
//     return 0;
// }

// long sys_access(const char *pathname, int mode) {
//     if (!pathname) return -EINVAL;
//     int ret = vfs_access(pathname, mode);
//     if (ret < 0) return ret;
//     return 0;
// }