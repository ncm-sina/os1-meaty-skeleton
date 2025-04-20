#include "stdio.h"

int scanf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vfscanf(stdin, format, args);
    va_end(args);
    return ret;
}

int vfscanf(FILE *stream, const char *format, va_list args) {
    // Reuse fscanf logic
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

    return assigned;
}