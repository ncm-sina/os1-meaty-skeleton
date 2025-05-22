// #ifndef _KERNEL_VFS_H
// #define _KERNEL_VFS_H

// #include <sys/types.h>
// #include <kernel/time.h>
// #include <errno.h>
// #include <stdint.h>
// #include <stddef.h>


// /* File open flags (matches POSIX) */
// #define O_RDONLY  0x0000
// #define O_WRONLY  0x0001
// #define O_RDWR    0x0002
// #define O_CREAT   0x0100
// #define O_EXCL    0x0200
// #define O_TRUNC   0x0400
// #define O_APPEND  0x0800

// // Constants
// #define VFS_FILE 1
// #define VFS_DIR  2
// #define DT_REG   8
// #define DT_DIR   4

// #define SEEK_SET 0
// #define SEEK_CUR 1
// #define SEEK_END 2

// #define R_OK 4
// #define W_OK 2
// #define X_OK 1

// #define ENOENT  2
// #define EIO     5
// #define EBADF   9
// #define ENOMEM 12
// #define EACCES 13
// #define EMFILE 24
// #define EINVAL 22
// #define ENOSYS 38


// // // POSIX types
// // typedef int mode_t;
// // typedef long off_t;

// // struct stat {
// //     uint32_t st_ino;     // Inode (cluster for FAT32)
// //     mode_t st_mode;      // File type, permissions
// //     uint32_t st_size;    // Size in bytes
// //     uint32_t st_mtime;   // Modification time
// // };

// // struct dirent {
// //     uint32_t d_ino;      // Inode (cluster)
// //     char d_name[256];    // File name
// //     uint8_t d_type;      // DT_DIR, DT_REG
// // };

// // struct vfs_dir {
// //     struct vfs_node *node; // Directory node
// //     size_t index;          // Current entry index
// //     void *private_data;    // FS-specific iterator
// // };

// // // File operations
// // struct file_operations {
// //     int (*open)(struct vfs_node *node, int flags, mode_t mode);
// //     int (*read)(struct vfs_node *node, void *buf, size_t count, off_t offset);
// //     int (*write)(struct vfs_node *node, const void *buf, size_t count, off_t offset);
// //     int (*readdir)(struct vfs_node *node, struct dirent *entry, size_t *index);
// //     int (*close)(struct vfs_node *node);
// //     int (*unlink)(struct vfs_node *node);
// //     int (*stat)(struct vfs_node *node, struct stat *statbuf);
// //     int (*access)(struct vfs_node *node, int mode);
// // };

// // // VFS node
// // struct vfs_node {
// //     char name[256];              // Node name
// //     uint32_t cluster;            // FAT32 cluster
// //     uint8_t type;                // VFS_FILE, VFS_DIR
// //     struct file_operations *ops;  // Operations
// //     void *private_data;          // FS-specific (fat32_fs)
// // };


// // VFS functions
// int vfs_init(void);
// int vfs_mount(const char *path, struct file_operations *ops, void *private_data);
// int vfs_open(const char *pathname, int flags, mode_t mode);
// int vfs_read(int fd, void *buf, size_t count);
// int vfs_write(int fd, const void *buf, size_t count);
// int vfs_unlink(const char *pathname);
// int vfs_close(int fd);
// int vfs_lseek(int fd, off_t offset, unsigned int origin);
// int vfs_stat(const char *pathname, struct stat *statbuf);
// int vfs_access(const char *pathname, int mode);
// struct vfs_dir *vfs_opendir(const char *pathname);
// int vfs_readdir(struct vfs_dir *dir, struct dirent *entry);
// int vfs_closedir(struct vfs_dir *dir);


// #endif
#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>
#include <kernel/time.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>


// POSIX types
typedef int mode_t;
typedef long off_t;

struct stat {
    uint32_t st_ino;     // Inode (cluster for FAT32)
    mode_t st_mode;      // File type, permissions
    uint32_t st_size;    // Size in bytes
    uint32_t st_mtime;   // Modification time
};

struct dirent {
    uint32_t d_ino;      // Inode (cluster)
    char d_name[256];    // File name
    uint8_t d_type;      // DT_DIR, DT_REG
};

struct vfs_dir {
    struct vfs_node *node; // Directory node
    size_t index;          // Current entry index
    void *private_data;    // FS-specific iterator
};

// File operations
struct file_operations {
    int (*open)(struct vfs_node *node, int flags, mode_t mode);
    int (*read)(struct vfs_node *node, void *buf, size_t count, off_t offset);
    int (*write)(struct vfs_node *node, const void *buf, size_t count, off_t offset);
    int (*readdir)(struct vfs_node *node, struct dirent *entry, size_t *index);
    int (*close)(struct vfs_node *node);
    int (*unlink)(struct vfs_node *node);
    int (*stat)(struct vfs_node *node, struct stat *statbuf);
    int (*access)(struct vfs_node *node, int mode);
};

// VFS node
struct vfs_node {
    char name[256];              // Node name
    uint32_t cluster;            // FAT32 cluster
    uint8_t type;                // VFS_FILE, VFS_DIR
    struct file_operations *ops;  // Operations
    void *private_data;          // FS-specific (fat32_fs)
};

// Constants
#define VFS_FILE 1
#define VFS_DIR  2
#define DT_REG   8
#define DT_DIR   4

/* File open flags (matches POSIX) */
#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_CREAT   0x0100
#define O_EXCL    0x0200
#define O_TRUNC   0x0400
#define O_APPEND  0x0800

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define R_OK 4
#define W_OK 2
#define X_OK 1

#define ENOENT  2
#define EIO     5
#define EBADF   9
#define ENOMEM 12
#define EACCES 13
#define EMFILE 24
#define EINVAL 22
#define ENOSYS 38

// VFS functions
int vfs_init(void);
int vfs_mount(const char *path, struct file_operations *ops, void *private_data);
int vfs_open(const char *pathname, int flags, mode_t mode);
int vfs_read(int fd, void *buf, size_t count);
int vfs_write(int fd, const void *buf, size_t count);
int vfs_unlink(const char *pathname);
int vfs_close(int fd);
int vfs_lseek(int fd, off_t offset, unsigned int origin);
int vfs_stat(const char *pathname, struct stat *statbuf);
int vfs_access(const char *pathname, int mode);
struct vfs_dir *vfs_opendir(const char *pathname);
int vfs_readdir(struct vfs_dir *dir, struct dirent *entry);
int vfs_closedir(struct vfs_dir *dir);

#endif