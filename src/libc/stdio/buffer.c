#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


// Helper function to convert an integer to a decimal string
static void itoa(int64_t num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    int64_t i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }

    int64_t temp = num;
    int64_t digits = 0;
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

static void utoa(uint64_t num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    uint64_t i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }

    uint64_t temp = num;
    uint64_t digits = 0;
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
static void itohex(uint64_t num, char *buf, int uppercase, int width, bool use_prefix) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[17]; // Max 16 digits for 64-bit hex + null
    int i = 0;
    int start = 0;

    if (num < 0) {
        buf[start++] = '-';
        num = -num;
    }

    if (num == 0) {
        temp[i++] = '0';
    } else {
        while (num > 0 && i<16) {
            temp[i++] = digits[num % 16];
            num /= 16;
        }
    }

    // int prefix_len = use_prefix ? 2 : 0; // "0x" or "0X" if prefix is used
    int hex_len = i; // Number of actual hex digits
    int total_len = width;//hex_len > width ? width : hex_len; // Total length including padding

    if (use_prefix) {
        buf[start++] = '0';
        buf[start++] = uppercase ? 'X' : 'x';
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

// // Helper function to convert an integer to a hexadecimal string
// static void itohex2(uint8_t* numm, char *buf, int uppercase, int width, bool use_prefix) {
//     int max_width = 16;
//     if(width>max_width)
//         width = max_width;

//     const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
//     char temp[17]; // Max 16 digits for 64-bit hex + null
//     int i = 0;
//     int start = 0;

//     while(i<width){
//         uint8_t num = * (numm + i/2);
//         if (num == 0) {
//             temp[i++] = '0';
//             temp[i++] = '0';
//         } else {
//             temp[i++] = digits[num / 16];
//             temp[i++] = digits[num % 16];
//         }    
//     }


//     // int prefix_len = use_prefix ? 2 : 0; // "0x" or "0X" if prefix is used
//     int hex_len = i; // Number of actual hex digits
//     int total_len = hex_len > width ? width : hex_len; // Total length including padding

//     if (use_prefix) {
//         buf[start++] = '0';
//         buf[start++] = uppercase ? 'X' : 'x';
//     }

//     for (int j = 0; j < total_len; j++) {
//         if (j < total_len - hex_len) {
//             buf[start + j] = '0'; // Leading zeros
//         } else {
//             buf[start + j] = temp[hex_len - (j - (total_len - hex_len)) - 1]; // Hex digits
//         }
//     }
//     buf[start + total_len] = '\0';
// }


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
int vfprintf(FILE *stream, const char *format, va_list args) {
    // Reuse fprintf logic (simplified)
    char buf[1024];
    int buf_pos = 0;

    for (int i = 0; format[i]; i++) {
        if (format[i] != '%') {
            if (buf_pos < 1024 - 1) buf[buf_pos++] = format[i];
            continue;
        }
        i++;
        if (!format[i]) break;

        if (buf_pos > 0) {
            if (write_buffer(stream, buf, buf_pos) < 0) return -1;
            buf_pos = 0;
        }

        switch (format[i]) {
            case 's': {
                char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                while (*s) {
                    if (buf_pos < 1024 - 1) buf[buf_pos++] = *s++;
                    if (buf_pos >= 1024 - 1) {
                        if (write_buffer(stream, buf, buf_pos) < 0) return -1;
                        buf_pos = 0;
                    }
                }
                break;
            }
            case 'd': {
                int d = va_arg(args, int);
                char num[16];
                int len = 0;
                if (d < 0) {
                    buf[buf_pos++] = '-';
                    d = -d;
                }
                do {
                    num[len++] = '0' + (d % 10);
                    d /= 10;
                } while (d);
                for (int j = len - 1; j >= 0; j--) {
                    if (buf_pos < 1024 - 1) buf[buf_pos++] = num[j];
                    if (buf_pos >= 1024 - 1) {
                        if (write_buffer(stream, buf, buf_pos) < 0) return -1;
                        buf_pos = 0;
                    }
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                if (buf_pos < 1024 - 1) buf[buf_pos++] = c;
                break;
            }
            default:
                if (buf_pos < 1024 - 1) buf[buf_pos++] = format[i];
        }
    }

    if (buf_pos > 0) {
        if (write_buffer(stream, buf, buf_pos) < 0) return -1;
    }

    return 0;
}
// Static function to format a string with arguments into output buffer
void format_string(char *output, const char *fstring, va_list args) {
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
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                case 's': {
                    const char *str = va_arg(args, const char *);
                    while (*str) *out++ = *str++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'U': {
                    uint64_t num = va_arg(args, uint64_t);
                    char num_buf[20];
                    utoa(num, num_buf);
                    char *p = num_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }                
                case 'u': {
                    uint32_t num = va_arg(args, uint32_t);
                    char num_buf[20];
                    utoa(num, num_buf);
                    char *p = num_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'D': {
                    int64_t num = va_arg(args, int64_t);
                    char num_buf[20];
                    itoa(num, num_buf);
                    char *p = num_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'd': {
                    int32_t num = va_arg(args, int32_t);
                    char num_buf[20];
                    itoa(num, num_buf);
                    char *p = num_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    *out++ = c;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'x': {
                    void* num = va_arg(args, uint32_t);
                    char hex_buf[20];
                    itohex(num, hex_buf, 0, width, use_prefix);
                    char *p = hex_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'X': {
                    uint64_t num = va_arg(args, uint64_t);
                    char hex_buf[20];
                    itohex(num, hex_buf, 1, width, use_prefix);
                    char *p = hex_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
                    break;
                }
                case 'f': {
                    double num = va_arg(args, double);
                    char float_buf[32];
                    dtoaf(num, float_buf, width, precision);
                    char *p = float_buf;
                    while (*p) *out++ = *p++;
                    width=0;
                    use_prefix=0;
                    precision=6;
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

int write_buffer(FILE *stream, const char *buf, int len) {
    if (!stream || !buf || len < 0 || stream->error) {
        errno = EINVAL;
        return -1;
    }
    if (stream->fd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Unbuffered: write directly */
    if (stream->buf_mode == _IONBF) {
        ssize_t written = write(stream->fd, buf, len);
        if (written < 0) {
            stream->error = 1;
            return -1;
        }
        return written;
    }

    /* Buffered: copy to buffer, flush if needed */
    int total_written = 0;
    while (len > 0) {
        /* Check for newline in line-buffered mode */
        int flush_newline = 0;
        if (stream->buf_mode == _IOLBF) {
            for (int i = 0; i < len; i++) {
                if (buf[i] == '\n') {
                    flush_newline = 1;
                    break;
                }
            }
        }

        /* Copy to buffer */
        size_t space = stream->buf_size - stream->buf_pos;
        size_t to_copy = len < space ? len : space;
        memcpy(stream->buffer + stream->buf_pos, buf, to_copy);
        stream->buf_pos += to_copy;
        buf += to_copy;
        len -= to_copy;
        total_written += to_copy;

        /* Flush if buffer full or newline in line-buffered mode */
        if (stream->buf_pos == stream->buf_size || flush_newline) {
            if (fflush(stream) < 0) {
                return -1;
            }
        }
    }

    return total_written;
}

int fflush(FILE *stream) {
    if (!stream || stream->error) {
        errno = EINVAL;
        return EOF;
    }
    if (stream->fd < 0 || stream->buf_mode == _IONBF) {
        return 0; /* Nothing to flush */
    }

    /* Write buffer contents */
    if (stream->buf_pos > 0) {
        ssize_t written = write(stream->fd, stream->buffer, stream->buf_pos);
        if (written < 0) {
            stream->error = 1;
            return EOF;
        }
        stream->buf_pos = 0; /* Reset buffer */
    }

    return 0;
}