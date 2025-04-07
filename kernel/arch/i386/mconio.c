#include <kernel/mconio.h>
#include <kernel/vga_basic.h>
#include <stdarg.h>

// Shared buffer for printf and cprintf
static char buffer[1024];

// Helper function to convert an integer to a decimal string
static void itoa(int num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    int i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }

    int temp = num;
    int digits = 0;
    while (temp > 0) {
        digits++;
        temp /= 10;
    }

    buf[i + digits] = '\0';
    while (num > 0) {
        buf[i + digits - 1] = (num % 10) + '0';
        num /= 10;
        digits--;
    }
}

// Helper function to convert an integer to a hexadecimal string
static void itohex(unsigned int num, char *buf, int uppercase, int width, bool use_prefix) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[9]; // Max 8 digits for 32-bit hex + null
    int i = 0;

    if (num == 0) {
        temp[i++] = '0';
    } else {
        while (num > 0) {
            temp[i++] = digits[num % 16];
            num /= 16;
        }
    }

    // int prefix_len = use_prefix ? 2 : 0; // "0x" or "0X" if prefix is used
    int hex_len = i; // Number of actual hex digits
    int total_len = hex_len > width ? hex_len : width; // Total length including padding
    int start = 0;

    if (use_prefix) {
        buf[0] = '0';
        buf[1] = uppercase ? 'X' : 'x';
        start = 2;
    }

    for (int j = 0; j < total_len; j++) {
        if (j < total_len - hex_len) {
            buf[start + j] = '0'; // Leading zeros
        } else {
            buf[start + j] = temp[hex_len - (j - (total_len - hex_len)) - 1]; // Hex digits
        }
    }
    buf[start + total_len] = '\0';
}

// Static function to format a string with arguments into output buffer
static void format_string(char *output, const char *fstring, va_list args) {
    char *out = output;
    while (*fstring) {
        if (*fstring == '%') {
            fstring++;
            if (*fstring == '\0') break;

            bool use_prefix = false;
            int width = 0;

            if (*fstring == '#') {
                use_prefix = true;
                fstring++;
            }

            while (*fstring >= '0' && *fstring <= '9') {
                width = width * 10 + (*fstring - '0');
                fstring++;
            }

            switch (*fstring) {
                case '%':
                    *out++ = '%';
                    break;
                case 's': {
                    const char *str = va_arg(args, const char *);
                    while (*str) *out++ = *str++;
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    char num_buf[12];
                    itoa(num, num_buf);
                    char *p = num_buf;
                    while (*p) *out++ = *p++;
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    *out++ = c;
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char hex_buf[11];
                    itohex(num, hex_buf, 0, width, use_prefix);
                    char *p = hex_buf;
                    while (*p) *out++ = *p++;
                    break;
                }
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    char hex_buf[11];
                    itohex(num, hex_buf, 1, width, use_prefix);
                    char *p = hex_buf;
                    while (*p) *out++ = *p++;
                    break;
                }
                default:
                    *out++ = '%';
                    *out++ = *fstring;
                    break;
            }
            fstring++;
        } else {
            *out++ = *fstring++;
        }
    }
    *out = '\0';
}

// Format a string into a caller-provided buffer (uses format_string)
void sprintf(char *output, const char *fstring, ...) {
    va_list args;
    va_start(args, fstring);
    format_string(output, fstring, args);
    va_end(args);
}

// Clear the screen
void clrscr(void) {
    _vgab_clear_screen();
}

// Print a formatted string with ANSI color support
void cprintf(const char *fstring, ...) {
    va_list args;
    va_start(args, fstring);
    format_string(buffer, fstring, args);
    va_end(args);

    unsigned char current_color = _vgab_get_textcolor();
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
                    }
                    i++;
                }
                if (valid && buffer[i] == 'm') {
                    if (num >= 30 && num <= 37) fg = num - 30;
                    else if (num >= 90 && num <= 97) fg = num - 90 + 8;
                    else if (num >= 40 && num <= 47) bg = num - 40;
                    else if (num >= 100 && num <= 107) bg = num - 100 + 8;
                    _vgab_set_textcolor(fg, bg);
                    i++;
                    continue;
                }
            }
            i++;
        }
        _vgab_put_char(buffer[i]);
        i++;
    }
}

// Print a formatted string without ANSI color support
void printf(const char *fstring, ...) {
    va_list args;
    va_start(args, fstring);
    format_string(buffer, fstring, args);
    va_end(args);

    _vgab_write_string(buffer);
}

// Print a single character
void putchar(char c) {
    _vgab_put_char(c);
}

// Set and get text color
void set_textcolor(uint8_t fg, uint8_t bg) {
    _vgab_set_textcolor(fg, bg);
}

uint8_t get_textcolor(void) {
    return _vgab_get_textcolor();
}

void set_text_fg_color(uint8_t fg) {
    uint8_t bg = (_vgab_get_textcolor() >> 4) & 0x0F;
    _vgab_set_textcolor(fg, bg);
}

uint8_t get_text_fg_color(void) {
    return _vgab_get_textcolor() & 0x0F;
}

void set_text_bg_color(uint8_t bg) {
    uint8_t fg = _vgab_get_textcolor() & 0x0F;
    _vgab_set_textcolor(fg, bg);
}

uint8_t get_text_bg_color(void) {
    return (_vgab_get_textcolor() >> 4) & 0x0F;
}

void gotoxy(uint8_t x, uint8_t y) {
    _vgab_set_cursor(x, y);
}