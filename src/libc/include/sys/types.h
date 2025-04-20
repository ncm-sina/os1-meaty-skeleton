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