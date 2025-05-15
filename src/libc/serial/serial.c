#include <stdarg.h>
#include <serial.h>

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
void serial_init(void) {
    // Disable interrupts
    outb(COM1_PORT + INT_ENABLE_REG, 0);

    // Set baud rate to 38400 (divisor = 115200 / 38400 = 3)
    outb(COM1_PORT + LINE_CONTROL_REG, 0x80); // Enable DLAB
    outb(COM1_PORT + BAUD_LOW, 0x03);         // Low byte of divisor
    outb(COM1_PORT + BAUD_HIGH, 0x00);        // High byte of divisor
    outb(COM1_PORT + LINE_CONTROL_REG, 0x03); // Disable DLAB, set 8N1

    // Enable FIFO, clear buffers, 14-byte threshold
    outb(COM1_PORT + 2, 0xC7);

    // Enable modem control (DTR, RTS, OUT2)
    outb(COM1_PORT + MODEM_CONTROL_REG, 0x0B);
}

// Check if transmit buffer is empty
static int serial_can_write(void) {
    return inb(COM1_PORT + LINE_STATUS_REG) & TX_EMPTY;
}

// Write a single character
void serial_putc(char c) {
    // Wait until transmit buffer is empty
    while (!serial_can_write());
    outb(COM1_PORT + DATA_REG, c);
}

// Write a null-terminated string
void serial_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            serial_putc('\r'); // Add carriage return before newline
        }
        serial_putc(*s++);
    }
}

// Simple printf-like function (supports %s, %d, %c, %x)
void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[16];
    int i = 0;

    while (fmt[i]) {
        if (fmt[i] != '%') {
            serial_putc(fmt[i++]);
            continue;
        }
        i++;
        switch (fmt[i]) {
            case 's': {
                const char *s = va_arg(args, const char *);
                serial_puts(s);
                break;
            }
            case 'd': {
                int num = va_arg(args, int);
                if (num < 0) {
                    serial_putc('-');
                    num = -num;
                }
                int j = 0;
                do {
                    buf[j++] = (num % 10) + '0';
                    num /= 10;
                } while (num);
                while (j--) {
                    serial_putc(buf[j]);
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                serial_putc(c);
                break;
            }
            case 'x': {
                unsigned int num = va_arg(args, unsigned int);
                int j = 0;
                do {
                    int digit = num % 16;
                    buf[j++] = digit < 10 ? digit + '0' : digit - 10 + 'a';
                    num /= 16;
                } while (num);
                while (j--) {
                    serial_putc(buf[j]);
                }
                break;
            }
            default:
                serial_putc(fmt[i]);
                break;
        }
        i++;
    }
    va_end(args);
}