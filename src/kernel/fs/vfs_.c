#include <kernel/fs/vfs.h>
#include <kernel/drivers/serial.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/utils/kmem.h>
// #include <errno.h>


#define MAX_FDS 32
#define MAX_PATH 256

struct fd_entry {
    struct vfs_node *node;
    off_t offset;
    int flags;
};

static struct vfs_node *root;
static struct fd_entry fd_table[MAX_FDS];

int vfs_init(void) {
    root = kmalloc(sizeof(struct vfs_node));
    if (!root) return -ENOMEM;
    root->name[0] = '\0';
    root->type = VFS_DIR;
    root->ops = NULL;
    root->cluster = 0;
    root->private_data = NULL;
    memset(fd_table, 0, sizeof(fd_table));
    return 0;
}

int vfs_mount(const char *path, struct file_operations *ops) {
    if (strcmp(path, "/") != 0) return -EINVAL;
    root->ops = ops;
    root->cluster = 2; // FAT32 root cluster
    root->private_data = NULL; // Set by FS
    return 0;
}

static struct vfs_node *vfs_resolve_path(const char *pathname) {
    // Simplified: only root supported
    if (strcmp(pathname, "/") == 0 || pathname[0] == '\0') {
        return root;
    }
    // TODO: Implement path parsing (e.g., /usr/bin)
    serial_printf("VFS: Path resolution for %s not implemented\n", pathname);
    return NULL;
}

int vfs_open(const char *pathname, int flags, mode_t mode) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    if (!node || !node->ops || !node->ops->open) return -ENOENT;
    int ret = node->ops->open(node, flags, mode);
    if (ret < 0) return ret;

    for (int i = 0; i < MAX_FDS; i++) {
        if (!fd_table[i].node) {
            fd_table[i].node = node;
            fd_table[i].offset = 0;
            fd_table[i].flags = flags;
            return i;
        }
    }
    return -EMFILE;
}

int vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].node) return -EBADF;
    struct vfs_node *node = fd_table[fd].node;
    if (!node->ops || !node->ops->read) return -EIO;
    int ret = node->ops->read(node, buf, count, fd_table[fd].offset);
    if (ret > 0) fd_table[fd].offset += ret;
    return ret;
}

int vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].node) return -EBADF;
    struct vfs_node *node = fd_table[fd].node;
    if (!node->ops || !node->ops->write) return -EIO;
    int ret = node->ops->write(node, buf, count, fd_table[fd].offset);
    if (ret > 0) fd_table[fd].offset += ret;
    return ret;
}

int vfs_unlink(const char *pathname) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    if (!node || !node->ops || !node->ops->unlink) return -ENOENT;
    return node->ops->unlink(node);
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].node) return -EBADF;
    struct vfs_node *node = fd_table[fd].node;
    if (!node->ops || !node->ops->close) return -EIO;
    int ret = node->ops->close(node);
    fd_table[fd].node = NULL;
    return ret;
}

int vfs_lseek(int fd, off_t offset, unsigned int origin) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].node) return -EBADF;
    off_t new_offset;
    switch (origin) {
        case SEEK_SET: new_offset = offset; break;
        case SEEK_CUR: new_offset = fd_table[fd].offset + offset; break;
        case SEEK_END: return -ENOSYS; // Requires stat
        default: return -EINVAL;
    }
    if (new_offset < 0) return -EINVAL;
    fd_table[fd].offset = new_offset;
    return new_offset;
}

int vfs_stat(const char *pathname, struct stat *statbuf) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    if (!node || !node->ops || !node->ops->stat) return -ENOENT;
    return node->ops->stat(node, statbuf);
}

int vfs_access(const char *pathname, int mode) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    if (!node || !node->ops || !node->ops->access) return -ENOENT;
    return node->ops->access(node, mode);
}

struct vfs_dir *vfs_opendir(const char *pathname) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    if (!node || !node->ops || !node->ops->readdir || node->type != VFS_DIR) return NULL;
    struct vfs_dir *dir = kmalloc(sizeof(struct vfs_dir));
    if (!dir) return NULL;
    dir->node = node;
    dir->index = 0;
    dir->private_data = NULL; // FS-specific
    return dir;
}

int vfs_readdir(struct vfs_dir *dir, struct dirent *entry) {
    if (!dir || !dir->node) return -EBADF;
    struct vfs_node *node = dir->node;
    if (!node->ops || !node->ops->readdir) return -EIO;
    return node->ops->readdir(node, entry, &dir->index);
}

int vfs_closedir(struct vfs_dir *dir) {
    if (!dir) return -EBADF;
    kfree(dir);
    return 0;
}

// #include <kernel/fs/vfs.h>
// #include <errno.h>

// int vfs_open(const char *pathname, int flags, mode_t mode) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_read(int fd, void *buf, size_t count) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_write(int fd, const void *buf, size_t count) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_unlink(const char *pathname) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_close(int fd) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_lseek(int fd, off_t offset, unsigned int origin) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_stat(const char *pathname, struct stat *statbuf) {
//     return -ENOSYS; /* TODO: Implement */
// }

// int vfs_access(const char *pathname, int mode) {
//     return -ENOSYS; /* TODO: Implement */
// }