#include <stdio.h>
#include <errno.h>

#if defined(__is_libk)

#include <kernel/drivers/vga.h>

#else

#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>
// extern int write_buffer(FILE *stream, const char *buf, int len);

#endif

int puts(const char *s) {
    if (!s) {
        errno = EINVAL;
        return EOF;
    }
    
    int len = 0;
    while (s[len]) len++;

#if defined(__is_libk)

    vga_write_string(s);
    vga_put_char('\n');

#else

    if (write_buffer(stdout, s, len) < 0) {
        return EOF;
    }
    if (write_buffer(stdout, "\n", 1) < 0) {
        return EOF;
    }
    
#endif

    return len + 1; // Includes newline

}