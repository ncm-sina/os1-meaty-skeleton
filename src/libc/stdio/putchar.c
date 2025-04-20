#include <stdio.h>
#include <errno.h>


#if defined(__is_libk)

#include <kernel/drivers/vga.h>

#else

#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

#endif

int putchar(char c) {
#if defined(__is_libk)

vga_put_char(c);

return c;

#else

return putc(c, stdout);

#endif
}

// #if defined(__is_libk)
// // #include <kernel/tty.h>
// #else

// #endif

// int putchar(int ic) {
// #if defined(__is_libk)

// #else

// #endif

