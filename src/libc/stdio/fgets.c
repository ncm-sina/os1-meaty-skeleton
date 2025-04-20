#include "stdio.h"
#include <errno.h>
#include "arch/i386/syscall_arch.h"
#include "arch/i386/bits/syscall.h"

char *fgets(char *s, int size, FILE *stream) {
    if (!s || size <= 0 || !stream) {
        errno = EINVAL;
        return NULL;
    }

    int pos = 0;
    while (pos < size - 1) {
        if (stream->buf_pos >= stream->buf_len) {
            int read = __syscall3(SYS_read, stream->fd, (long)stream->buffer, stream->buf_size);
            if (read <= 0) {
                if (read == 0) stream->eof = 1;
                else stream->error = 1;
                if (pos == 0) return NULL;
                break;
            }
            stream->buf_pos = 0;
            stream->buf_len = read;
        }

        char c = stream->buffer[stream->buf_pos++];
        s[pos++] = c;
        if (c == '\n') break;
    }
    s[pos] = '\0';

    return pos > 0 ? s : NULL;
}