#include "stdio.h"
#include <errno.h>
#include "arch/i386/syscall_arch.h"
#include "arch/i386/bits/syscall.h"

int getc(FILE *stream) {
    if (!stream) {
        errno = EINVAL;
        return EOF;
    }

    if (stream->buf_pos >= stream->buf_len) {
        int read = __syscall3(SYS_read, stream->fd, (long)stream->buffer, stream->buf_size);
        if (read <= 0) {
            if (read == 0) stream->eof = 1;
            else stream->error = 1;
            return EOF;
        }
        stream->buf_pos = 0;
        stream->buf_len = read;
    }

    return (unsigned char)stream->buffer[stream->buf_pos++];
}