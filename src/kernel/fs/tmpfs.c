#include <kernel/fs/tmpfs.h>
#include <errno.h>
#include <kernel/mm/syscall.h>

static struct fs_ops tmpfs_ops = {
    .open = tmpfs_open,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .unlink = tmpfs_unlink
};

static struct superblock *root_sb;

static int tmpfs_open(struct inode *inode, struct file *file) {
    file->f_inode = inode;
    file->f_pos = 0;
    return 0;
}

static int tmpfs_read(struct file *file, void *buf, size_t count) {
    return -ENOSYS; /* TODO: Implement */
}

static int tmpfs_write(struct file *file, const void *buf, size_t count) {
    return -ENOSYS; /* TODO: Implement */
}

static int tmpfs_unlink(struct inode *dir, const char *name) {
    return -ENOSYS; /* TODO: Implement */
}

struct superblock *tmpfs_mount(void) {
    struct superblock *sb = kmalloc(sizeof(struct superblock));
    if (!sb) return NULL;
    sb->s_root = kmalloc(sizeof(struct inode));
    if (!sb->s_root) {
        kfree(sb);
        return NULL;
    }
    sb->s_root->i_mode = S_IFDIR | 0755;
    sb->s_root->i_uid = 0;
    sb->s_root->i_gid = 0;
    sb->s_root->i_size = 0;
    sb->s_root->i_atime = 0;
    sb->s_root->i_mtime = 0;
    sb->s_root->i_ctime = 0;
    sb->s_root->i_sb = sb;
    sb->s_private = NULL;
    return sb;
}

void tmpfs_init(void) {
    root_sb = tmpfs_mount();
    if (!root_sb) {
        /* Handle error */
    }
}