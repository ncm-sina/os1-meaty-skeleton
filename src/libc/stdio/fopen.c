#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>
#include <sys/mman.h>

FILE *fopen(const char *path, const char *mode) {
    int flags = 0, sys_mode = 0666; // Default permissions
    if (!path || !mode) {
        errno = EINVAL;
        return NULL;
    }

    // Parse mode (simplified: "r", "w", "a", no "+")
    if (mode[0] == 'r') {
        flags = O_RDONLY;
    } else if (mode[0] == 'w') {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (mode[0] == 'a') {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        errno = EINVAL;
        return NULL;
    }

    int fd = __syscall3(SYS_open, (long)path, flags, sys_mode);
    if (fd < 0) {
        errno = -fd;
        return NULL; // errno set by syscall
    }

    void *addr = mmap(NULL, sizeof(FILE) + BUFSIZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // FILE *stream = (FILE *)__syscall3(SYS_mmap, 0, sizeof(FILE) + BUFSIZ, 3, 34, -1, 0);
    if ((long)addr == -1) {
        __syscall1(SYS_close, fd);
        errno = ENOMEM;
        return NULL;
    }

    FILE *stream = (FILE *)addr;
    stream->fd = fd;
    stream->mode = flags & (O_RDONLY | O_WRONLY | O_RDWR);
    stream->flags = (flags & O_RDONLY) ? _IO_READ : _IO_WRITE;
    stream->buffer = (char *)(stream + 1);
    stream->buf_size = BUFSIZ;
    stream->buf_pos = 0;
    stream->buf_mode = _IOFBF;
    stream->pos = 0;
    stream->error = 0;
    stream->eof = 0;


    return stream;
}