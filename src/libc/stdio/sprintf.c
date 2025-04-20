#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

int sprintf(char *output, const char *format, ...) {
    if (!output || !format) {
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_start(args, format);
    format_string(output, format, args);
    va_end(args);

    int pos = 0;
    pos = strlen(output);

    return pos;
}