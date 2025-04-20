#include <stdio.h>
#include <errno.h>
#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

#include <kernel/fs/vfs.h>
#include <unistd.h>

int fclose(FILE *stream) {
    if (!stream || stream == stdin || stream == stdout || stream == stderr) {
        errno = EINVAL;
        return -1;
    }

    // Flush buffer for writable streams
    if (stream->mode & (O_WRONLY | O_RDWR) && stream->buf_pos > 0) {
        int written = write( stream->fd, (long)stream->buffer, stream->buf_pos);
        if (written < 0) {
            stream->error = 1;
            return -1;
        }
    }

    int ret = close( stream->fd);
    __syscall2(SYS_munmap, (long)stream, sizeof(FILE) + BUFSIZ);
    return ret;
}