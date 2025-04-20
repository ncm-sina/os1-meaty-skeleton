#ifndef _SYS_UIO_H
#define _SYS_UIO_H

// #include <sys/cdefs.h>

// #include <stddef.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Vector I/O functions */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#ifdef __cplusplus
}
#endif

#endif
