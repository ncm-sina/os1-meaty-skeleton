#include <stdarg.h>
#include "serial.h"

// I/O port definitions for COM1
#define COM1_PORT 0x3F8

// Serial port registers (offsets from COM1_PORT)
#define DATA_REG 0          // Data register (write bytes here)
#define INT_ENABLE_REG 1    // Interrupt enable
#define BAUD_LOW 0          // Baud rate divisor (low byte)
#define BAUD_HIGH 1         // Baud rate divisor (high byte)
#define LINE_CONTROL_REG 3  // Line control register
#define MODEM_CONTROL_REG 4 // Modem control register
#define LINE_STATUS_REG 5   // Line status register

// Line status bits
#define TX_EMPTY 0x20 // Transmit buffer empty

// I/O port functions
static inline void outb(unsigned short port, unsigned char val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Initialize COM1: 38400 baud, 8N1 (8 bits, no parity, 1 stop bit)
int serial_init(void) {
    outb(COM1_PORT + INT_ENABLE_REG, 0);
    outb(COM1_PORT + LINE_CONTROL_REG, 0x80); // Enable DLAB
    outb(COM1_PORT + BAUD_LOW, 0x03);         // Low byte of divisor
    outb(COM1_PORT + BAUD_HIGH, 0x00);        // High byte of divisor
    outb(COM1_PORT + LINE_CONTROL_REG, 0x03); // Disable DLAB, set 8N1
    outb(COM1_PORT + 2, 0xC7);                // Enable FIFO
    outb(COM1_PORT + MODEM_CONTROL_REG, 0x0B);

    return 0;
}

// Check if transmit buffer is empty
static int serial_can_write(void) {
    return inb(COM1_PORT + LINE_STATUS_REG) & TX_EMPTY;
}

// Write a single character
void serial_putc(char c) {
    while (!serial_can_write());
    outb(COM1_PORT + DATA_REG, c);
}

// Write a null-terminated string
void serial_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            serial_putc('\r');
        }
        serial_putc(*s++);
    }
}

// Helper: Convert number to string
static void itoa(char *buf, unsigned long num, int base, int width, int zero_pad, int alt_form, int is_signed, int is_negative) {
    char temp[64];
    int i = 0;
    const char *digits = "0123456789abcdef";
    int prefix_len = 0;

    if (num == 0) {
        temp[i++] = '0';
    } else {
        while (num) {
            temp[i++] = digits[num % base];
            num /= base;
        }
    }

    int num_len = i;
    if (alt_form && base == 16 && num_len > 0) {
        prefix_len = 2; // For "0x"
    }
    if (is_negative) {
        prefix_len++; // For "-"
    }

    // Calculate padding
    int total_len = num_len + prefix_len;
    int pad_len = width > total_len ? width - total_len : 0;
    int pos = 0;

    // Add prefix
    if (is_negative) {
        buf[pos++] = '-';
    }
    if (alt_form && base == 16 && num_len > 0) {
        buf[pos++] = '0';
        buf[pos++] = 'x';
    }

    // Add padding
    while (pad_len-- > 0) {
        buf[pos++] = zero_pad ? '0' : ' ';
    }

    // Reverse digits
    while (i--) {
        buf[pos++] = temp[i];
    }
    buf[pos] = '\0';
}

// Enhanced serial_printf
void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[64];

    while (*fmt) {
        if (*fmt != '%') {
            serial_putc(*fmt++);
            continue;
        }
        fmt++;

        // Parse flags
        int zero_pad = 0;
        int alt_form = 0;
        while (*fmt == '0' || *fmt == '#') {
            if (*fmt == '0') zero_pad = 1;
            if (*fmt == '#') alt_form = 1;
            fmt++;
        }

        // Parse width
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // Parse precision
        int precision = -1;
        if (*fmt == '.') {
            fmt++;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                fmt++;
            }
        }

        // Handle specifiers
        switch (*fmt) {
            case 'c': {
                char c = (char)va_arg(args, int);
                serial_putc(c);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                int len = 0;
                while (s[len] && (precision < 0 || len < precision)) len++;
                int pad_len = width > len ? width - len : 0;
                while (pad_len--) serial_putc(' '); // Right-align
                for (int i = 0; i < len; i++) serial_putc(s[i]);
                break;
            }
            case 'd': {
                long num = va_arg(args, int);
                int is_negative = num < 0;
                if (is_negative && num == -2147483648) {
                    serial_puts("-2147483648");
                } else {
                    itoa(buf, is_negative ? -num : num, 10, width, zero_pad, 0, 1, is_negative);
                    serial_puts(buf);
                }
                break;
            }
            case 'u': {
                unsigned long num = va_arg(args, unsigned int);
                itoa(buf, num, 10, width, zero_pad, 0, 0, 0);
                serial_puts(buf);
                break;
            }
            case 'x': {
                unsigned long num = va_arg(args, unsigned int);
                itoa(buf, num, 16, width, zero_pad, alt_form, 0, 0);
                serial_puts(buf);
                break;
            }
            case 'p': {
                unsigned long num = (unsigned long)va_arg(args, void *);
                int pad_width = width < 10 ? 10 : width; // Minimum 8 digits + 0x
                itoa(buf, num, 16, pad_width, 1, 1, 0, 0);
                serial_puts(buf);
                break;
            }
            case '%': {
                serial_putc('%');
                break;
            }
            default: {
                serial_putc('%');
                if (*fmt) serial_putc(*fmt);
                break;
            }
        }
        if (*fmt) fmt++;
    }
    va_end(args);
}