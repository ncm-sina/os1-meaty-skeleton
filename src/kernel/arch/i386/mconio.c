#include <kernel/mconio.h>
#include <kernel/drivers/vga.h>
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

// Helper function to convert a double to a string with specified width and precision
static void dtoaf(double num, char *buf, int width, int precision) {
    int i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }

    // Integer part
    int int_part = (int)num;
    double frac_part = num - (double)int_part;

    // Convert integer part
    char int_buf[12];
    itoa(int_part, int_buf);
    int int_len = 0;
    while (int_buf[int_len]) int_len++;

    // Calculate total length (sign + integer + decimal + precision)
    int sign_len = (num < 0) ? 1 : 0;
    int total_min_len = sign_len + int_len + 1 + precision; // sign + integer + '.' + precision
    int pad_len = (width > total_min_len) ? (width - total_min_len) : 0;

    // Add padding spaces
    for (int j = 0; j < pad_len; j++) {
        buf[i++] = ' ';
    }

    // Copy integer part
    char *p = int_buf;
    while (*p) {
        buf[i++] = *p++;
    }

    // Decimal point
    buf[i++] = '.';

    // Fractional part
    if (precision == 0) {
        buf[i] = '\0';
        return;
    }

    // Multiply by 10^precision and round
    double multiplier = 1.0;
    for (int j = 0; j < precision; j++) {
        multiplier *= 10.0;
    }
    int frac_int = (int)(frac_part * multiplier + 0.5);

    if (frac_int == 0) {
        for (int j = 0; j < precision; j++) {
            buf[i++] = '0';
        }
    } else {
        char frac_buf[16]; // Enough for large precision
        itoa(frac_int, frac_buf);
        int len = 0;
        while (frac_buf[len]) len++;
        for (int j = 0; j < precision - len; j++) {
            buf[i++] = '0'; // Leading zeros
        }
        p = frac_buf;
        while (*p) {
            buf[i++] = *p++;
        }
    }

    buf[i] = '\0';
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
            int precision = 6; // Default precision for %f

            if (*fstring == '#') {
                use_prefix = true;
                fstring++;
            }

            // Parse width (e.g., "6" in "%6.2f")
            while (*fstring >= '0' && *fstring <= '9') {
                width = width * 10 + (*fstring - '0');
                fstring++;
            }

            // Parse precision (e.g., "2" in "%6.2f")
            if (*fstring == '.') {
                fstring++;
                precision = 0; // Reset default if '.' is present
                while (*fstring >= '0' && *fstring <= '9') {
                    precision = precision * 10 + (*fstring - '0');
                    fstring++;
                }
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
                case 'f': {
                    double num = va_arg(args, double);
                    char float_buf[32];
                    dtoaf(num, float_buf, width, precision);
                    char *p = float_buf;
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
    vga_clear();
}

// Print a formatted string with ANSI color support
void cprintf(const char *fstring, ...) {
    va_list args;
    va_start(args, fstring);
    format_string(buffer, fstring, args);
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
        vga_put_char(buffer[i]);
        i++;
    }
}

// Print a formatted string without ANSI color support
void printf(const char *fstring, ...) {
    va_list args;
    va_start(args, fstring);
    format_string(buffer, fstring, args);
    va_end(args);

    vga_write_string(buffer);
}

// Print a single character
void putchar(char c) {
    vga_put_char(c);
}

// Set and get text color
void set_textcolor(uint8_t fg, uint8_t bg) {
    vga_set_textcolor(fg, bg);
}

uint8_t get_textcolor(void) {
    return vga_get_textcolor();
}

void set_text_fg_color(uint8_t fg) {
    uint8_t bg = (vga_get_textcolor() >> 4) & 0x0F;
    vga_set_textcolor(fg, bg);
}

uint8_t get_text_fg_color(void) {
    return vga_get_textcolor() & 0x0F;
}

void set_text_bg_color(uint8_t bg) {
    uint8_t fg = vga_get_textcolor() & 0x0F;
    vga_set_textcolor(fg, bg);
}

uint8_t get_text_bg_color(void) {
    return (vga_get_textcolor() >> 4) & 0x0F;
}

void gotoxy(uint8_t x, uint8_t y) {
    vga_gotoxy(x, y);
}