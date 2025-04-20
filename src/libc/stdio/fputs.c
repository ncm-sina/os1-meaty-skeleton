#include "stdio.h"
#include <errno.h>
#include "arch/i386/syscall_arch.h"
#include "arch/i386/bits/syscall.h"

int fputs(const char *s, FILE *stream) {
    if (!s || !stream) {
        errno = EINVAL;
        return EOF;
    }

    int len = 0;
    while (s[len]) len++;

    if (write_buffer(stream, s, len) < 0) {
        return EOF;
    }

    return len;
}