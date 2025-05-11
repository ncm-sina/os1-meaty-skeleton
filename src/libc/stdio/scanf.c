#include <stdio.h>

#include <stdarg.h>
#include <unistd.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/vga.h>


#if defined(__is_libk)
// extern VGA_TEXTINFO vga_textinfo;


// Minimal string/ctype functions (replace with your implementations if available)
static int isdigit(char c) { return c >= '0' && c <= '9'; }
static int isspace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static int isxdigit(char c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static int toupper(int c) { if (c >= 'a' && c <= 'z') return c - 32; return c; }

static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void *memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void *memset(void *s, int c, size_t n) {
    char *p = s;
    while (n--) *p++ = c;
    return s;
}

// Number conversion (replace with your strtol/strtoul if available)
static long strtol(const char *s, char **endptr, int base) {
    long result = 0;
    int sign = 1;
    while (isspace(*s)) s++;
    if (*s == '-') { sign = -1; s++; } else if (*s == '+') s++;
    if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') base = 8;
        else base = 10;
    }
    while (*s) {
        int digit;
        if (isdigit(*s)) digit = *s - '0';
        else if (isxdigit(*s)) digit = toupper(*s) - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }
    if (endptr) *endptr = (char *)s;
    return result * sign;
}

static unsigned long strtoul(const char *s, char **endptr, int base) {
    unsigned long result = 0;
    while (isspace(*s)) s++;
    if (*s == '+') s++;
    if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') base = 8;
        else base = 10;
    }
    while (*s) {
        int digit;
        if (isdigit(*s)) digit = *s - '0';
        else if (isxdigit(*s)) digit = toupper(*s) - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }
    if (endptr) *endptr = (char *)s;
    return result;
}

// Input buffer
#define INPUT_BUFFER_SIZE 256
static char input_buffer[INPUT_BUFFER_SIZE];
static size_t input_pos = 0;

// Get a single character from the keyboard, echoing to VGA
static int getchar(void) {
    while (1) {
        struct key_event event;
        int res = keyboard_drv.get_event(&event);
        while (!res) {
            // printf("r:%d,c:%d ", res, event.code);
            res = keyboard_drv.get_event(&event);
                // asm("hlt"); // Wait for interrupt
        }
        // printf("%02x ", event.code);
        if (event.type != KEY_PRESS) continue; // Ignore releases
        char c = keyboard_drv.keycode_to_ascii(event.code, event.modifiers);
        if (!c && event.code != KEY_ENTER && event.code != KEY_BACKSPACE) continue; // Ignore non-printable
        if (event.code == KEY_ENTER) c = '\n';

        if (event.code == KEY_BACKSPACE && input_pos > 0) {
            input_pos--;
            input_buffer[input_pos] = '\0';
            CORDS pos = vga_get_cursor();
            if (pos.x > 0) {
                vga_gotoxy(pos.x - 1, pos.y);
                vga_put_char(' ');
                vga_gotoxy(pos.x - 1, pos.y);
            } else if (pos.y > 0) {
                vga_gotoxy(VGA_WIDTH - 1, pos.y - 1);
                vga_put_char(' ');
                vga_gotoxy(VGA_WIDTH - 1, pos.y - 1);
            }
        } else if (c && input_pos < INPUT_BUFFER_SIZE - 1) {
            input_buffer[input_pos++] = c;
            input_buffer[input_pos] = '\0';
            vga_put_char(c);
        }
        return c;
    }
}

// Collect input until newline
static int read_input(void) {
    input_pos = 0;
    memset(input_buffer, 0, INPUT_BUFFER_SIZE);
    while (input_pos < INPUT_BUFFER_SIZE - 1) {
        int c = getchar();
        if (c == '\n') break; // Stop on newline
        if (c < 0) return EOF; // Critical error
    }
    return input_pos ? input_pos : EOF;
}

// Kernel-level scanf
int kscanf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int assigned = 0;
    size_t fmt_pos = 0;
    char *input_ptr = input_buffer;

    if (read_input() == EOF && input_pos == 0) {
        va_end(args);
        // printf("--ip:%d ", input_pos);
        return EOF;
    }
    // printf("+ip:%d ,buf: %s", input_pos, input_buffer);

    while (format[fmt_pos] && *input_ptr) {
        if (isspace(format[fmt_pos])) {
            while (isspace(*input_ptr)) input_ptr++;
            fmt_pos++;
            continue;
        }
        if (format[fmt_pos] != '%') {
            if (*input_ptr != format[fmt_pos]) break;
            input_ptr++;
            fmt_pos++;
            continue;
        }
        fmt_pos++; // Skip '%'

        // Parse width
        int width = 0;
        while (isdigit(format[fmt_pos])) {
            width = width * 10 + (format[fmt_pos] - '0');
            fmt_pos++;
        }

        // Parse length modifier
        int length = 0; // 0=default, 1=short, 2=long, 3=long long
        if (format[fmt_pos] == 'h') { length = 1; fmt_pos++; }
        else if (format[fmt_pos] == 'l') { length = 2; fmt_pos++; if (format[fmt_pos] == 'l') { length = 3; fmt_pos++; } }

        // Parse specifier
        char spec = format[fmt_pos++];
        char *endptr;
        while (isspace(*input_ptr)) input_ptr++; // Skip leading whitespace

        if (spec == 'c') {
            if (!*input_ptr) break;
            char *dest = va_arg(args, char *);
            *dest = *input_ptr++;
            assigned++;
        } else if (spec == 's') {
            char *dest = va_arg(args, char *);
            size_t i = 0;
            while (*input_ptr && !isspace(*input_ptr) && (!width || i < width)) {
                dest[i++] = *input_ptr++;
            }
            dest[i] = '\0';
            if (i > 0) assigned++;
        } else if (spec == 'd' || spec == 'i') {
            // printf("xxxx:%d %s", input_pos, input_ptr);
            if (!*input_ptr) break;
            long val = strtol(input_ptr, &endptr, spec == 'i' ? 0 : 10);
            // printf("val: %d ", val);
            if (endptr == input_ptr) break;
            // printf("xcxcx:%d", length);
            if (length == 1) *va_arg(args, short *) = (short)val;
            else if (length == 2) *va_arg(args, long *) = val;
            else if (length == 3) *va_arg(args, long long *) = (long long)val;
            else *va_arg(args, int *) = (int)val;
            input_ptr = endptr;
            assigned++;
        } else if (spec == 'u') {
            if (!*input_ptr) break;
            unsigned long val = strtoul(input_ptr, &endptr, 10);
            if (endptr == input_ptr) break;
            if (length == 1) *va_arg(args, unsigned short *) = (unsigned short)val;
            else if (length == 2) *va_arg(args, unsigned long *) = val;
            else if (length == 3) *va_arg(args, unsigned long long *) = (unsigned long long)val;
            else *va_arg(args, unsigned int *) = (unsigned int)val;
            input_ptr = endptr;
            assigned++;
        } else if (spec == 'x') {
            if (!*input_ptr) break;
            unsigned long val = strtoul(input_ptr, &endptr, 16);
            if (endptr == input_ptr) break;
            if (length == 1) *va_arg(args, unsigned short *) = (unsigned short)val;
            else if (length == 2) *va_arg(args, unsigned long *) = val;
            else if (length == 3) *va_arg(args, unsigned long long *) = (unsigned long long)val;
            else *va_arg(args, unsigned int *) = (unsigned int)val;
            input_ptr = endptr;
            assigned++;
        } else if (spec == 'o') {
            if (!*input_ptr) break;
            unsigned long val = strtoul(input_ptr, &endptr, 8);
            if (endptr == input_ptr) break;
            if (length == 1) *va_arg(args, unsigned short *) = (unsigned short)val;
            else if (length == 2) *va_arg(args, unsigned long *) = val;
            else if (length == 3) *va_arg(args, unsigned long long *) = (unsigned long long)val;
            else *va_arg(args, unsigned int *) = (unsigned int)val;
            input_ptr = endptr;
            assigned++;
        } else if (spec == 'p') {
            if (!*input_ptr) break;
            unsigned long val = strtoul(input_ptr, &endptr, 16);
            if (endptr == input_ptr) break;
            *va_arg(args, void **) = (void *)val;
            input_ptr = endptr;
            assigned++;
        } else if (spec == 'f') {
            // Stub: Floating-point not supported
            va_arg(args, double *); // Advance argument
        } else {
            break; // Unknown specifier
        }
    }

    va_end(args);
    return assigned;
}

// User-space scanf (stub)
int scanf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = kscanf(format, args);
    va_end(args);
    return result;
}

#else

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
#endif
