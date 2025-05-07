#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#if defined(__is_libk)

// #include <kernel/drivers/vga.h>
#include <kernel/drivers/vbe.h>

#else

#include <arch/i386/syscall_arch.h>
#include <arch/i386/bits/syscall.h>

#endif


// int printf(const char *fmt, ...) {
//     char buf[1024];
//     va_list args;
//     va_start(args, fmt);
//     int len = vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);
//     return write(1, buf, len);
// }

int printf2(const char *format, ...) {
    #if defined(__is_libk)
    char buffer[1024];

    va_list args;
    va_start(args, format);
    format_string(buffer, format, args);
    va_end(args);

    unsigned char current_color = vga_get_textcolor();
    size_t i = 0;

    while (buffer[i] != '\0') {
        if (buffer[i] == '\033' || buffer[i] == '\x1b') {
            if (buffer[i + 1] == '[') {
                i += 2;
                uint8_t fg = current_color & 0x0F;
                uint8_t bg = (current_color >> 4) & 0x0F;
                int num = 0;
                bool valid = false;

                while (buffer[i] != 'm' && buffer[i] != '\0') {
                    if (buffer[i] >= '0' && buffer[i] <= '9') {
                        num = num * 10 + (buffer[i] - '0');
                        valid = true;
                    } else if (buffer[i] == ';') {
                        if (num >= 30 && num <= 37) fg = num - 30;
                        else if (num >= 90 && num <= 97) fg = num - 90 + 8;
                        else if (num >= 40 && num <= 47) bg = num - 40;
                        else if (num >= 100 && num <= 107) bg = num - 100 + 8;
                        num = 0;
                        valid=false;
                    }
                    i++;
                }
                if (valid && buffer[i] == 'm') {
                    if (num >= 30 && num <= 37) fg = num - 30;
                    else if (num >= 90 && num <= 97) fg = num - 90 + 8;
                    else if (num >= 40 && num <= 47) bg = num - 40;
                    else if (num >= 100 && num <= 107) bg = num - 100 + 8;
                    vga_set_textcolor(fg, bg);
                    i++;
                    continue;
                }
            }
            i++;
        }
        vbe_putchar(buffer[i]);
        i++;
    }
    int ret;
    ret = i;

#else
    
va_list args;
va_start(args, format);

int ret = vfprintf(stdout, format, args);
va_end(args);

#endif
    return ret;
}

