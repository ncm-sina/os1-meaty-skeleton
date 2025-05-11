#include <string.h>

/* Structure to hold format specifier information */
typedef struct {
    int left_align;   /* 1 if left-aligned, 0 if right-aligned */
    int zero_pad;     /* 1 if zero-padded, 0 if space-padded */
    size_t width;     /* Field width */
    size_t precision; /* Precision for strings or numbers */
    char length;      /* Length modifier: 'l' or 'L' (ll) */
    char specifier;   /* Format specifier: d, u, x, X, s, c, p */
} FormatSpec;

/* Helper: Parse format specifier */
static const char *parse_format(const char *fmt, FormatSpec *spec, va_list *args) {
    spec->left_align = 0;
    spec->zero_pad = 0;
    spec->width = 0;
    spec->precision = -1; /* -1 means unspecified */
    spec->length = 0;
    spec->specifier = 0;

    /* Parse flags */
    while (*fmt == '-' || *fmt == '0') {
        if (*fmt == '-') spec->left_align = 1;
        if (*fmt == '0') spec->zero_pad = 1;
        fmt++;
    }

    /* Parse width */
    if (*fmt == '*') {
        spec->width = va_arg(*args, int);
        if (spec->width < 0) {
            spec->left_align = 1;
            spec->width = -spec->width;
        }
        fmt++;
    } else {
        while (*fmt >= '0' && *fmt <= '9') {
            spec->width = spec->width * 10 + (*fmt - '0');
            fmt++;
        }
    }

    /* Parse precision */
    if (*fmt == '.') {
        fmt++;
        spec->precision = 0;
        if (*fmt == '*') {
            spec->precision = va_arg(*args, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                spec->precision = spec->precision * 10 + (*fmt - '0');
                fmt++;
            }
        }
    }

    /* Parse length modifier */
    if (*fmt == 'l') {
        spec->length = 'l';
        fmt++;
        if (*fmt == 'l') {
            spec->length = 'L'; /* ll */
            fmt++;
        }
    }

    /* Parse specifier */
    spec->specifier = *fmt;
    if (*fmt) fmt++;
    return fmt;
}

/* Helper: Output a single character to buffer */
static int put_char(char *str, size_t size, size_t *pos, char c) {
    if (*pos < size - 1) {
        str[*pos] = c;
    }
    (*pos)++;
    return 1;
}

/* Helper: Output a string to buffer with padding */
static int put_string(char *str, size_t size, size_t *pos, const char *s, FormatSpec *spec) {
    size_t len = 0;
    size_t i;
    const char *p = s;
    while (*p && (spec->precision == (size_t)-1 || len < spec->precision)) {
        len++;
        p++;
    }

    size_t pad = (spec->width > len) ? spec->width - len : 0;
    char pad_char = spec->zero_pad && !spec->left_align ? '0' : ' ';

    if (!spec->left_align) {
        for (i = 0; i < pad; i++) {
            put_char(str, size, pos, pad_char);
        }
    }

    for (i = 0; i < len && s[i]; i++) {
        put_char(str, size, pos, s[i]);
    }

    if (spec->left_align) {
        for (i = 0; i < pad; i++) {
            put_char(str, size, pos, ' ');
        }
    }

    return len + pad;
}

/* Helper: Convert number to string and output */
static int put_number(char *str, size_t size, size_t *pos, unsigned long long value, int is_signed, FormatSpec *spec) {
    char buf[32];
    char *p = buf + 31;
    *p = '\0';
    int base = 10;
    char *digits = (spec->specifier == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
    int is_negative = 0;

    if (is_signed && (long long)value < 0) {
        is_negative = 1;
        value = -(long long)value;
    }

    if (spec->specifier == 'x' || spec->specifier == 'X' || spec->specifier == 'p') {
        base = 16;
    }

    do {
        *--p = digits[value % base];
        value /= base;
    } while (value);

    if (is_negative) {
        *--p = '-';
    } else if (spec->specifier == 'p') {
        *--p = 'x';
        *--p = '0';
    }

    size_t len = buf + 31 - p;
    size_t prec = (spec->precision != (size_t)-1) ? spec->precision : 0;
    size_t zero_pad = (prec > len) ? prec - len : 0;
    size_t width_pad = (spec->width > len + zero_pad) ? spec->width - (len + zero_pad) : 0;
    char pad_char = spec->zero_pad && !spec->left_align ? '0' : ' ';

    if (!spec->left_align) {
        for (size_t i = 0; i < width_pad; i++) {
            put_char(str, size, pos, pad_char);
        }
    }

    for (size_t i = 0; i < zero_pad; i++) {
        put_char(str, size, pos, '0');
    }

    while (*p) {
        put_char(str, size, pos, *p++);
    }

    if (spec->left_align) {
        for (size_t i = 0; i < width_pad; i++) {
            put_char(str, size, pos, ' ');
        }
    }

    return len + zero_pad + width_pad;
}

/* Sophisticated vsnprintf implementation */
int vsnprintf(char *str, size_t size, const char *format, va_list args) {
    size_t pos = 0;
    FormatSpec spec;

    if (size == 0) {
        return 0;
    }

    while (*format) {
        if (*format != '%') {
            put_char(str, size, &pos, *format++);
            continue;
        }
        format++;
        format = parse_format(format, &spec, &args);

        switch (spec.specifier) {
            case 's': {
                char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                put_string(str, size, &pos, s, &spec);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                put_char(str, size, &pos, c);
                break;
            }
            case 'd': {
                if (spec.length == 'L') {
                    long long val = va_arg(args, long long);
                    put_number(str, size, &pos, val, 1, &spec);
                } else {
                    int val = va_arg(args, int);
                    put_number(str, size, &pos, val, 1, &spec);
                }
                break;
            }
            case 'u': {
                if (spec.length == 'L') {
                    unsigned long long val = va_arg(args, unsigned long long);
                    put_number(str, size, &pos, val, 0, &spec);
                } else {
                    unsigned int val = va_arg(args, unsigned int);
                    put_number(str, size, &pos, val, 0, &spec);
                }
                break;
            }
            case 'x':
            case 'X': {
                if (spec.length == 'L') {
                    unsigned long long val = va_arg(args, unsigned long long);
                    put_number(str, size, &pos, val, 0, &spec);
                } else {
                    unsigned int val = va_arg(args, unsigned int);
                    put_number(str, size, &pos, val, 0, &spec);
                }
                break;
            }
            case 'p': {
                unsigned long val = (unsigned long)va_arg(args, void *);
                put_number(str, size, &pos, val, 0, &spec);
                break;
            }
            default: {
                put_char(str, size, &pos, spec.specifier);
                break;
            }
        }
    }

    if (pos < size) {
        str[pos] = '\0';
    } else if (size > 0) {
            str[size - 1] = '\0';
    }


    return pos;
}

/* Sophisticated snprintf implementation */
int snprintf(char *str, unsigned int size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}
// /* Helper function to convert an integer to a string */
// static int itoa(char *buf, int value, int base, int is_signed) {
//     char temp[32];
//     char *p = temp;
//     int is_negative = 0;
//     unsigned int uvalue;

//     if (is_signed && value < 0) {
//         is_negative = 1;
//         uvalue = -value;
//     } else {
//         uvalue = (unsigned int)value;
//     }

//     do {
//         int digit = uvalue % base;
//         *p++ = (digit < 10) ? digit + '0' : digit - 10 + 'a';
//         uvalue /= base;
//     } while (uvalue);

//     if (is_negative) {
//         *p++ = '-';
//     }

//     int len = 0;
//     while (p > temp && len < 31) {
//         *buf++ = *--p;
//         len++;
//     }
//     *buf = '\0';
//     return len;
// }

// /* Simplified vsnprintf implementation for basic format specifiers */
// static int vsnprintf(char *str, unsigned int size, const char *format, va_list args) {
//     unsigned int i = 0;
//     char *s;
//     int len;

//     if (size == 0) {
//         return 0;
//     }

//     while (*format && i < size - 1) {
//         if (*format != '%') {
//             str[i++] = *format++;
//             continue;
//         }
//         format++;
//         switch (*format) {
//             case 's':
//                 s = va_arg(args, char *);
//                 while (*s && i < size - 1) {
//                     str[i++] = *s++;
//                 }
//                 format++;
//                 break;
//             case 'd':
//                 len = itoa(str + i, va_arg(args, int), 10, 1);
//                 i += len;
//                 format++;
//                 break;
//             case 'u':
//                 len = itoa(str + i, va_arg(args, unsigned int), 10, 0);
//                 i += len;
//                 format++;
//                 break;
//             case 'c':
//                 str[i++] = (char)va_arg(args, int);
//                 format++;
//                 break;
//             default:
//                 str[i++] = *format++;
//                 break;
//         }
//     }
//     str[i] = '\0';
//     return i;
// }

// /* Writes formatted string to str, up to size characters */
// int snprintf(char *str, unsigned int size, const char *format, ...) {
//     va_list args;
//     va_start(args, format);
//     int ret = vsnprintf(str, size, format, args);
//     va_end(args);
//     return ret;
// }