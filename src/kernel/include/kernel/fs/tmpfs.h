#ifndef _KERNEL_FS_TMPFS_H
#define _KERNEL_FS_TMPFS_H

#include <kernel/vfs.h>

void tmpfs_init(void);
struct superblock *tmpfs_mount(void);

#endif