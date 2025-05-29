#include <kernel/fs/vfs.h>
#include <kernel/drivers/serial.h>
#include <stdlib.h>
#include <string.h>

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
    // serial_printf(" 1 ");    
    root = kmalloc(sizeof(struct vfs_node));
    // serial_printf(" 2 ");    
    if (!root) return -ENOMEM;
    // serial_printf(" 3 ");    
    root->name[0] = '\0';
    root->type = VFS_DIR;
    root->ops = NULL;
    root->cluster = 0;
    root->private_data = NULL;
    // serial_printf(" 4 ");    
    memset(fd_table, 0, sizeof(fd_table));
    // serial_printf(" 5 ");    
    return 0;
}

int vfs_mount(const char *path, struct file_operations *ops, void *private_data) {
    if (strcmp(path, "/") != 0) return -EINVAL;
    root->ops = ops;
    root->cluster = 2; // FAT32 root cluster
    root->private_data = private_data; // Set by FS
    return 0;
}

static struct vfs_node *vfs_lookup(struct vfs_node *dir, const char *name) {
    // serial_printf(" d:%s vfs_node:%s %d ", name, dir->name, dir->type);
    if (dir->type != VFS_DIR || !dir->ops->readdir) return NULL;
    // serial_printf(" vl1 ");
    struct vfs_dir dir_stream = { .node = dir, .index = 0, .private_data = NULL };
    struct dirent entry;
    while (dir->ops->readdir(dir, &entry, &dir_stream.index)) {
        // serial_printf(" vl2 r: %s\n", entry.d_name);
        if (stricmp(entry.d_name, name) == 0) {
            struct vfs_node *node = kmalloc(sizeof(struct vfs_node));
            if (!node) return NULL;
            strncpy(node->name, entry.d_name, 255);
            node->name[255] = '\0';
            node->cluster = entry.d_ino;
            node->type = (entry.d_type == DT_DIR) ? VFS_DIR : VFS_FILE;
            node->ops = dir->ops;
            node->private_data = dir->private_data;
            return node;
        }
    }
    return NULL;
}


static struct vfs_node *vfs_resolve_path(const char *pathname) {
    // serial_printf(" q ");
    if (!root->ops || !root->ops->readdir) return NULL;
    // serial_printf(" w ");

    // Handle root
    if (strcmp(pathname, "/") == 0 || pathname[0] == '\0') {
        return root;
    }
    // serial_printf(" e ");

    // Basic path parsing (e.g., /usr)
    char path_copy[MAX_PATH];
    strncpy(path_copy, pathname, MAX_PATH - 1);
    path_copy[MAX_PATH - 1] = '\0';
    // serial_printf(" r:path_copy:%s \n", path_copy);

    // Skip leading /
    char *component = path_copy;
    if (component[0] == '/') component++;
    // serial_printf(" t ");

    struct vfs_node *current = root;
    char *next_slash;
    while ((next_slash = strchr(component, '/'))) {
        *next_slash = '\0'; // Isolate component
        // serial_printf(" c:%s ", component);
        current = vfs_lookup(current, component);
        if (!current){
            return NULL;
        }
        component = next_slash + 1;
    }
    // serial_printf(" y ");

    // Handle final component
    if (*component) {
        current = vfs_lookup(current, component);
    }
    // serial_printf(" u ");

    return current;
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
    kfree(node); // Free if no FD available
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
    int ret = node->ops->unlink(node);
    kfree(node);
    return ret;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].node) return -EBADF;
    struct vfs_node *node = fd_table[fd].node;
    if (!node->ops || !node->ops->close) return -EIO;
    int ret = node->ops->close(node);
    kfree(node);
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
    int ret = node->ops->stat(node, statbuf);
    kfree(node);
    return ret;
}

int vfs_access(const char *pathname, int mode) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    if (!node || !node->ops || !node->ops->access) return -ENOENT;
    int ret = node->ops->access(node, mode);
    kfree(node);
    return ret;
}

struct vfs_dir *vfs_opendir(const char *pathname) {
    struct vfs_node *node = vfs_resolve_path(pathname);
    // serial_printf(" m:%s, %s, %d, %d", pathname, node->name, node->cluster, node->type);
    if (!node || !node->ops || !node->ops->readdir || node->type != VFS_DIR) {
        // serial_printf(" n ");
        if (node) kfree(node);
        return NULL;
    }
    struct vfs_dir *dir = kmalloc(sizeof(struct vfs_dir));
    // serial_printf(" k ");
    if (!dir) {
        // serial_printf(" v ");
        kfree(node);
        return NULL;
    }
    // serial_printf(" h ");
    dir->node = node;
    dir->index = 0;
    dir->private_data = NULL;
    return dir;
}

int vfs_readdir(struct vfs_dir *dir, struct dirent *entry) {
    if (!dir || !dir->node) return -EBADF;
    struct vfs_node *node = dir->node;
    if (!node->ops || !node->ops->readdir) return -EIO;
    return node->ops->readdir(node, entry, &dir->index);
}

int vfs_closedir(struct vfs_dir *dir) {
    if (!dir || !dir->node) return -EBADF;
    kfree(dir->node);
    kfree(dir);
    return 0;
}