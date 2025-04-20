#include "stdio.h"
#include <stdarg.h>
#include <errno.h>
#include "arch/i386/syscall_arch.h"
#include "arch/i386/bits/syscall.h"

static int read_buffer(FILE *stream, char *buf, int size) {
    if (!(stream->mode & (O_RDONLY | O_RDWR))) {
        errno = EBADF;
        return -1;
    }

    if (stream->buf_pos >= stream->buf_len) {
        int read = __syscall3(SYS_read, stream->fd, (long)stream->buffer, stream->buf_size);
        if (read <= 0) {
            if (read == 0) stream->eof = 1;
            else stream->error = 1;
            return read;
        }
        stream->buf_pos = 0;
        stream->buf_len = read;
    }

    int count = 0;
    while (count < size && stream->buf_pos < stream->buf_len) {
        buf[count++] = stream->buffer[stream->buf_pos++];
    }
    return count;
}

int fscanf(FILE *stream, const char *format, ...) {
    if (!stream || !format) {
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_start(args, format);

    char buf[1024];
    int buf_pos = 0;
    int buf_len = 0;
    int assigned = 0;

    for (int i = 0; format[i]; i++) {
        if (format[i] != '%') {
            if (buf_pos >= buf_len) {
                buf_len = read_buffer(stream, buf, 1024);
                if (buf_len <= 0) break;
                buf_pos = 0;
            }
            if (buf[buf_pos] != format[i]) break;
            buf_pos++;
            continue;
        }
        i++;
        if (!format[i]) break;

        if (buf_pos >= buf_len) {
            buf_len = read_buffer(stream, buf, 1024);
            if (buf_len <= 0) break;
            buf_pos = 0;
        }

        switch (format[i]) {
            case 's': {
                char *s = va_arg(args, char *);
                int j = 0;
                while (buf_pos < buf_len && buf[buf_pos] != ' ' && buf[buf_pos] != '\n') {
                    s[j++] = buf[buf_pos++];
                    if (buf_pos >= buf_len) {
                        buf_len = read_buffer(stream, buf, 1024);
                        if (buf_len <= 0) break;
                        buf_pos = 0;
                    }
                }
                s[j] = '\0';
                assigned++;
                break;
            }
            case 'd': {
                int *d = va_arg(args, int *);
                int num = 0;
                int sign = 1;
                if (buf[buf_pos] == '-') {
                    sign = -1;
                    buf_pos++;
                }
                while (buf_pos < buf_len && buf[buf_pos] >= '0' && buf[buf_pos] <= '9') {
                    num = num * 10 + (buf[buf_pos] - '0');
                    buf_pos++;
                    if (buf_pos >= buf_len) {
                        buf_len = read_buffer(stream, buf, 1024);
                        if (buf_len <= 0) break;
                        buf_pos = 0;
                    }
                }
                *d = num * sign;
                assigned++;
                break;
            }
            case 'c': {
                char *c = va_arg(args, char *);
                *c = buf[buf_pos++];
                assigned++;
                break;
            }
        }
    }

    va_end(args);
    return assigned;
}