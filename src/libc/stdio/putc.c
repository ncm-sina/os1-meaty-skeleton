#include <stdio.h>
#include <errno.h>


#if defined(__is_libk)
// #include <kernel/tty.h>
#else

#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

#endif

int putc(int c, FILE *stream) {

#if defined(__is_libk)

#else
if (!stream) {
    errno = EINVAL;
    return EOF;
}

if (stream->buf_pos >= stream->buf_size) {
    int written = __syscall3(SYS_write, stream->fd, (long)stream->buffer, stream->buf_pos);
    if (written < 0) {
        stream->error = 1;
        return EOF;
    }
    stream->buf_pos = 0;
}

stream->buffer[stream->buf_pos++] = c;
if (c == '\n' && stream == stdout) {
    int written = __syscall3(SYS_write, stream->fd, (long)stream->buffer, stream->buf_pos);
    if (written < 0) {
        stream->error = 1;
        return EOF;
    }
    stream->buf_pos = 0;
}

return c;

#endif


}