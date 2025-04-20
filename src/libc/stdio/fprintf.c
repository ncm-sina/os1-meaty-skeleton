#include "stdio.h"
#include <stdarg.h>
#include <errno.h>
#include "arch/i386/syscall_arch.h"
#include "arch/i386/bits/syscall.h"

static int write_buffer(FILE *stream, const char *buf, int len) {
    if (!(stream->mode & (O_WRONLY | O_RDWR))) {
        errno = EBADF;
        return -1;
    }

    // Buffer data
    for (int i = 0; i < len; i++) {
        if (stream->buf_pos >= stream->buf_size) {
            int written = __syscall3(SYS_write, stream->fd, (long)stream->buffer, stream->buf_pos);
            if (written < 0) {
                stream->error = 1;
                return -1;
            }
            stream->buf_pos = 0;
        }
        stream->buffer[stream->buf_pos++] = buf[i];
        if (buf[i] == '\n' && stream == stdout) { // Line buffering for stdout
            int written = __syscall3(SYS_write, stream->fd, (long)stream->buffer, stream->buf_pos);
            if (written < 0) {
                stream->error = 1;
                return -1;
            }
            stream->buf_pos = 0;
        }
    }
    return len;
}

int fprintf(FILE *stream, const char *format, ...) {
    if (!stream || !format) {
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_start(args, format);

    char buf[1024]; // Temporary buffer for formatting
    int buf_pos = 0;

    for (int i = 0; format[i]; i++) {
        if (format[i] != '%') {
            if (buf_pos < 1024 - 1) buf[buf_pos++] = format[i];
            continue;
        }
        i++;
        if (!format[i]) break;

        // Flush buffer if full
        if (buf_pos > 0) {
            if (write_buffer(stream, buf, buf_pos) < 0) {
                va_end(args);
                return -1;
            }
            buf_pos = 0;
        }

        switch (format[i]) {
            case 's': {
                char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                while (*s) {
                    if (buf_pos < 1024 - 1) buf[buf_pos++] = *s++;
                    if (buf_pos >= 1024 - 1) {
                        if (write_buffer(stream, buf, buf_pos) < 0) {
                            va_end(args);
                            return -1;
                        }
                        buf_pos = 0;
                    }
                }
                break;
            }
            case 'd': {
                int d = va_arg(args, int);
                char num[16];
                int len = 0;
                if (d < 0) {
                    buf[buf_pos++] = '-';
                    d = -d;
                }
                do {
                    num[len++] = '0' + (d % 10);
                    d /= 10;
                } while (d);
                for (int j = len - 1; j >= 0; j--) {
                    if (buf_pos < 1024 - 1) buf[buf_pos++] = num[j];
                    if (buf_pos >= 1024 - 1) {
                        if (write_buffer(stream, buf, buf_pos) < 0) {
                            va_end(args);
                            return -1;
                        }
                        buf_pos = 0;
                    }
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                if (buf_pos < 1024 - 1) buf[buf_pos++] = c;
                break;
            }
            default:
                if (buf_pos < 1024 - 1) buf[buf_pos++] = format[i];
        }
    }

    if (buf_pos > 0) {
        if (write_buffer(stream, buf, buf_pos) < 0) {
            va_end(args);
            return -1;
        }
    }

    va_end(args);
    return 0; // Return number of chars written (simplified)
}