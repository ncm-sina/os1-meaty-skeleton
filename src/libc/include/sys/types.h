#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Integer types for system calls */
typedef unsigned int    mode_t;   /* File mode (permissions) */
typedef unsigned int    dev_t;
typedef unsigned int    ino_t;
typedef long            off_t;    /* File offset */
typedef unsigned long   size_t;   /* Size of objects */
typedef long            ssize_t;  /* Signed size (for return values) */
typedef int             pid_t;    /* Process ID */
typedef unsigned int    uid_t;    /* User ID */
typedef unsigned int    gid_t;    /* Group ID */
/* Time in seconds */
typedef long time_t;
typedef long clock_t;

/* Structs for stat, sigaction, etc. */
struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;    /* File mode */
    off_t  st_size;    /* File size */
    uid_t  st_uid;     /* Owner UID */
    gid_t  st_gid;     /* Owner GID */
    uint32_t st_mtime;   // Modification time
    /* Add more fields as needed */
};


/* For writev */
struct iovec {
    void  *iov_base;   /* Base address */
    size_t iov_len;    /* Length */
};

/* For uname */
struct utsname {
    char sysname[65];  /* OS name */
    char nodename[65]; /* Node name */
    char release[65];  /* Release level */
    char version[65];  /* Version level */
    char machine[65];  /* Hardware type */
};

/* Nanosecond resolution time */
struct timespec {
    time_t tv_sec;  /* Seconds */
    long   tv_nsec; /* Nanoseconds */
};

/* Microsecond resolution time */
struct timeval {
    time_t tv_sec;  /* Seconds */
    long   tv_usec; /* Microseconds */
};

/* Timezone information */
struct timezone {
    int tz_minuteswest; /* Minutes west of Greenwich */
    int tz_dsttime;     /* Type of DST correction */
};

struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};

/* File descriptor structure */
struct file {
    struct inode *f_inode;  /* Associated inode */
    off_t f_pos;            /* Current file offset */
    int f_flags;            /* Open flags (O_RDONLY, etc.) */
};

/* Inode structure */
struct inode {
    mode_t i_mode;          /* File type and permissions */
    uid_t i_uid;            /* Owner user ID */
    gid_t i_gid;            /* Owner group ID */
    off_t i_size;           /* File size */
    time_t i_atime;         /* Last access time */
    time_t i_mtime;         /* Last modification time */
    time_t i_ctime;         /* Creation time */
    struct superblock *i_sb; /* Superblock */
    void *i_private;        /* File-system-specific data */
};

/* Superblock structure */
struct superblock {
    struct inode *s_root;   /* Root inode */
    void *s_private;        /* File-system-specific data */
    const struct fs_ops *s_ops; /* File system operations */
};

// POSIX types
// typedef int mode_t;
// typedef long off_t;

// struct stat {
//     uint32_t st_ino;     // Inode (cluster for FAT32)
//     mode_t st_mode;      // File type, permissions
//     uint32_t st_size;    // Size in bytes
//     uint32_t st_mtime;   // Modification time
// };

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


/* File system operations */
struct fs_ops {
    int (*open)(struct inode *inode, struct file *file);
    int (*read)(struct file *file, void *buf, size_t count);
    int (*write)(struct file *file, const void *buf, size_t count);
    int (*unlink)(struct inode *dir, const char *name);
    int (*lseek)(struct file *file, off_t offset, unsigned int origin);
    int (*stat)(struct inode *inode, struct stat *statbuf);
};


#endif