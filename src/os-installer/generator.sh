#!/bin/bash

# Script to create the MyOS installer folder structure and files

# Base directory
BASE_DIR="os-installer"

# Function to create a file with content
create_file() {
    local file_path="$1"
    local content="$2"
    local dir_path=$(dirname "$file_path")
    
    # Create directory if it doesn't exist
    mkdir -p "$dir_path" || { echo "Error: Failed to create directory $dir_path"; exit 1; }
    
    # Write content to file
    echo "$content" > "$file_path" || { echo "Error: Failed to write to $file_path"; exit 1; }
    echo "Created $file_path"
}

# Create directories
mkdir -p "$BASE_DIR/boot/grub" \
         "$BASE_DIR/src/pages/welcome" \
         "$BASE_DIR/src/pages/tos" \
         "$BASE_DIR/src/pages/setup" \
         "$BASE_DIR/src/pages/create_partition" \
         "$BASE_DIR/src/pages/delete_partition" \
         "$BASE_DIR/src/pages/format_partition" \
         "$BASE_DIR/src/pages/installing" \
         "$BASE_DIR/src/drivers" \
         "$BASE_DIR/src/lib" \
         "$BASE_DIR/src/utils" \
         "$BASE_DIR/src/interrupt" \
         "$BASE_DIR/iso" || { echo "Error: Failed to create directories"; exit 1; }

# Create linker.ld
create_file "$BASE_DIR/linker.ld" \
"OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x100000; /* Load at 1MB */
    .text : {
        *(.multiboot)
        *(.text)
    }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}"

# Create Makefile
create_file "$BASE_DIR/Makefile" \
"CC = i686-elf-gcc
LD = i686-elf-ld
CFLAGS = -ffreestanding -m32 -g -Wall -Wextra -nostdlib -fno-builtin
LDFLAGS = -T linker.ld
OBJCOPY = i686-elf-objcopy

SRC_DIR = src
OBJ_DIR = obj
ISO_DIR = iso
BOOT_DIR = boot

SOURCES = \$(wildcard \$(SRC_DIR)/*.c) \\
          \$(wildcard \$(SRC_DIR)/drivers/*.c) \\
          \$(wildcard \$(SRC_DIR)/lib/*.c) \\
          \$(wildcard \$(SRC_DIR)/utils/*.c) \\
          \$(wildcard \$(SRC_DIR)/interrupt/*.c) \\
          \$(wildcard \$(SRC_DIR)/pages/*/*.c)
ASM_SOURCES = \$(SRC_DIR)/multiboot.S
OBJECTS = \$(patsubst \$(SRC_DIR)/%.S,\$(OBJ_DIR)/%.o,\$(ASM_SOURCES)) \\
          \$(patsubst \$(SRC_DIR)/%.c,\$(OBJ_DIR)/%.o,\$(SOURCES))

TARGET = \$(ISO_DIR)/boot/installer.elf
ISO = myos-installer.iso

all: \$(ISO)

\$(OBJ_DIR)/%.o: \$(SRC_DIR)/%.c
	@mkdir -p \$(@D)
	\$(CC) \$(CFLAGS) -c \$< -o \$@

\$(OBJ_DIR)/%.o: \$(SRC_DIR)/%.S
	@mkdir -p \$(@D)
	\$(CC) \$(CFLAGS) -c \$< -o \$@

\$(TARGET): \$(OBJECTS)
	@mkdir -p \$(ISO_DIR)/boot
	\$(LD) \$(LDFLAGS) \$(OBJECTS) -o \$@

\$(ISO): \$(TARGET) \$(BOOT_DIR)/grub/grub.cfg
	@mkdir -p \$(ISO_DIR)/boot/grub
	cp \$(BOOT_DIR)/grub/grub.cfg \$(ISO_DIR)/boot/grub/
	grub-mkrescue -o \$(ISO) \$(ISO_DIR)

clean:
	rm -rf \$(OBJ_DIR) \$(ISO_DIR) \$(ISO)

.PHONY: all clean"

# Create boot/grub/grub.cfg
create_file "$BASE_DIR/boot/grub/grub.cfg" \
"set timeout=5
set default=0

menuentry \"MyOS Installer\" {
    multiboot /boot/installer.elf
    boot
}"

# Create src/multiboot.S
create_file "$BASE_DIR/src/multiboot.S" \
".section .multiboot
.align 4
.long 0x1BADB002          /* Magic number */
.long 0x00000000          /* Flags */
.long -(0x1BADB002)       /* Checksum */

.section .text
.global _start
_start:
    cli                   /* Disable interrupts */
    movl \$stack_top, %esp /* Set up stack */
    pushl %ebx            /* Multiboot info */
    pushl %eax            /* Multiboot magic */
    call main             /* Call C entry point */
    hlt                   /* Halt if main returns */

.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16KB stack */
stack_top:"

# Create src/main.c
create_file "$BASE_DIR/src/main.c" \
"#include \"drivers/vga.h\"
#include \"drivers/keyboard.h\"
#include \"drivers/timer.h\"
#include \"interrupt/idt.h\"
#include \"pages/welcome/welcome_page.h\"

const char* OS_NAME = \"MyOS\"; /* Global OS name */

/* Multiboot structure (simplified) */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} multiboot_info_t;

void main(uint32_t magic, multiboot_info_t* mbi) {
    (void)magic; (void)mbi; /* Unused for now */
    
    /* Initialize drivers */
    idt_init();           /* Set up IDT for interrupts */
    timer_init(100);      /* 100 Hz for timer */
    keyboard_init();      /* Enable keyboard interrupts */
    vga_init();           /* Set up VGA with default colors */
    
    /* Start with welcome page */
    welcome_page();
    
    /* Should not reach here */
    for(;;) asm(\"hlt\");
}"

# Create src/drivers/vga.h
create_file "$BASE_DIR/src/drivers/vga.h" \
"#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef enum {
    COLOR_BLACK = 0, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
    COLOR_RED, COLOR_MAGENTA, COLOR_BROWN, COLOR_LIGHT_GREY,
    COLOR_DARK_GREY, COLOR_LIGHT_BLUE, COLOR_LIGHT_GREEN,
    COLOR_LIGHT_CYAN, COLOR_LIGHT_RED, COLOR_LIGHT_MAGENTA,
    COLOR_YELLOW, COLOR_WHITE
} vga_color_t;

void vga_init(void);
void clrscr(void);
void gotoxy(uint8_t x, uint8_t y);
void set_textcolor(vga_color_t bg, vga_color_t fg);

#endif"

# Create src/drivers/vga.c
create_file "$BASE_DIR/src/drivers/vga.c" \
"#include \"vga.h\"
#include <stdint.h>

static uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static vga_color_t current_bg = COLOR_BLACK;
static vga_color_t current_fg = COLOR_WHITE;

/* Initialize VGA with default colors */
void vga_init(void) {
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    clrscr();
}

/* Clear screen with current colors */
void clrscr(void) {
    uint16_t blank = ' ' | (current_bg << 12) | (current_fg << 8);
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
    gotoxy(0, 0);
}

/* Set cursor position */
void gotoxy(uint8_t x, uint8_t y) {
    cursor_x = x < VGA_WIDTH ? x : VGA_WIDTH - 1;
    cursor_y = y < VGA_HEIGHT ? y : VGA_HEIGHT - 1;
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 14);
    outb(0x3D5, pos >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}

/* Set background and foreground colors */
void set_textcolor(vga_color_t bg, vga_color_t fg) {
    current_bg = bg;
    current_fg = fg;
}

/* Helper: Write character at cursor */
void vga_put_char(char c) {
    if (c == '\\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        uint16_t attrib = (current_bg << 12) | (current_fg << 8);
        VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] = c | attrib;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    if (cursor_y >= VGA_HEIGHT) {
        /* Scroll up */
        for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
        }
        uint16_t blank = ' ' | (current_bg << 12) | (current_fg << 8);
        for (size_t i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            VGA_BUFFER[i] = blank;
        }
        cursor_y = VGA_HEIGHT - 1;
    }
    gotoxy(cursor_x, cursor_y);
}

/* I/O port helpers */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile(\"outb %0, %1\" : : \"a\"(val), \"Nd\"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile(\"inb %1, %0\" : \"=a\"(ret) : \"Nd\"(port));
    return ret;
}"

# Create src/drivers/keyboard.h
create_file "$BASE_DIR/src/drivers/keyboard.h" \
"#ifndef KEYBOARD_H
#define KEYBOARD_H

#define INPUT_BUFFER_SIZE 100

typedef enum {
    KEY_A = 0x1E, KEY_B = 0x30, KEY_C = 0x2E, KEY_D = 0x20, KEY_E = 0x12,
    KEY_F = 0x21, KEY_G = 0x22, KEY_H = 0x23, KEY_I = 0x17, KEY_J = 0x24,
    KEY_K = 0x25, KEY_L = 0x26, KEY_M = 0x32, KEY_N = 0x31, KEY_O = 0x18,
    KEY_P = 0x19, KEY_Q = 0x10, KEY_R = 0x13, KEY_S = 0x1F, KEY_T = 0x14,
    KEY_U = 0x16, KEY_V = 0x2F, KEY_W = 0x11, KEY_X = 0x2D, KEY_Y = 0x15,
    KEY_Z = 0x2C,
    KEY_0 = 0x0B, KEY_1 = 0x02, KEY_2 = 0x03, KEY_3 = 0x04, KEY_4 = 0x05,
    KEY_5 = 0x06, KEY_6 = 0x07, KEY_7 = 0x08, KEY_8 = 0x09, KEY_9 = 0x0A,
    KEY_ENTER = 0x1C, KEY_ESC = 0x01, KEY_BACKSPACE = 0x0E,
    KEY_F1 = 0x3B, KEY_F2 = 0x3C, KEY_F3 = 0x3D, KEY_F4 = 0x3E,
    KEY_F5 = 0x3F, KEY_F6 = 0x40, KEY_F7 = 0x41, KEY_F8 = 0x42,
    KEY_F9 = 0x43, KEY_F10 = 0x44, KEY_F11 = 0x57, KEY_F12 = 0x58,
    KEY_UP = 0x48, KEY_DOWN = 0x50, KEY_LEFT = 0x4B, KEY_RIGHT = 0x4D
} keycode_t;

void keyboard_init(void);
char getchar(void);
void scanf(const char* format, ...);
const char* keycode_to_name(keycode_t code);

#endif"

# Create src/drivers/keyboard.c
create_file "$BASE_DIR/src/drivers/keyboard.c" \
"#include \"keyboard.h\"
#include \"vga.h\"
#include \"interrupt/idt.h\"
#include <stdarg.h>
#include <stdint.h>

static char input_buffer[INPUT_BUFFER_SIZE];
static size_t input_len = 0;
static volatile uint8_t key_pressed = 0;
static volatile keycode_t last_keycode = 0;

/* Keycode to ASCII mapping (simplified, only for A-Z, 0-9) */
static const char keycode_to_ascii[] = {
    [KEY_A] = 'A', [KEY_B] = 'B', [KEY_C] = 'C', [KEY_D] = 'D', [KEY_E] = 'E',
    [KEY_F] = 'F', [KEY_G] = 'G', [KEY_H] = 'H', [KEY_I] = 'I', [KEY_J] = 'J',
    [KEY_K] = 'K', [KEY_L] = 'L', [KEY_M] = 'M', [KEY_N] = 'N', [KEY_O] = 'O',
    [KEY_P] = 'P', [KEY_Q] = 'Q', [KEY_R] = 'R', [KEY_S] = 'S', [KEY_T] = 'T',
    [KEY_U] = 'U', [KEY_V] = 'V', [KEY_W] = 'W', [KEY_X] = 'X', [KEY_Y] = 'Y',
    [KEY_Z] = 'Z',
    [KEY_0] = '0', [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_4] = '4',
    [KEY_5] = '5', [KEY_6] = '6', [KEY_7] = '7', [KEY_8] = '8', [KEY_9] = '9',
    [KEY_ENTER] = '\\n', [KEY_ESC] = 27, [KEY_BACKSPACE] = '\\b'
};

/* Keycode to name for action bar */
const char* keycode_to_name(keycode_t code) {
    switch (code) {
        case KEY_ENTER: return \"Enter\";
        case KEY_ESC: return \"Esc\";
        case KEY_F1: return \"F1\";
        case KEY_F2: return \"F2\";
        case KEY_F3: return \"F3\";
        case KEY_F4: return \"F4\";
        case KEY_F5: return \"F5\";
        case KEY_F6: return \"F6\";
        case KEY_F7: return \"F7\";
        case KEY_F8: return \"F8\";
        case KEY_F9: return \"F9\";
        case KEY_F10: return \"F10\";
        case KEY_F11: return \"F11\";
        case KEY_F12: return \"F12\";
        case KEY_UP: return \"Up\";
        case KEY_DOWN: return \"Down\";
        case KEY_LEFT: return \"Left\";
        case KEY_RIGHT: return \"Right\";
        case KEY_C: return \"C\";
        case KEY_D: return \"D\";
        case KEY_F: return \"F\";
        default: return \"Unknown\";
    }
}

/* Initialize keyboard (enable IRQ1) */
void keyboard_init(void) {
    outb(0x21, inb(0x21) & ~0x02); /* Enable IRQ1 */
}

/* Keyboard interrupt handler */
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    if (!(scancode & 0x80)) { /* Key press */
        last_keycode = scancode;
        key_pressed = 1;
    }
    outb(0x20, 0x20); /* EOI */
}

/* Get single character (blocks until keypress) */
char getchar(void) {
    while (!key_pressed) asm(\"hlt\");
    key_pressed = 0;
    return keycode_to_ascii[last_keycode];
}

/* scanf implementation with blinking cursor */
void scanf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    input_len = 0;
    input_buffer[0] = '\\0';
    uint8_t cursor_visible = 1;
    uint32_t last_blink = timer_ticks;
    
    while (1) {
        /* Blink cursor at BLINK_SPEED */
        if (timer_ticks - last_blink >= BLINK_SPEED) {
            cursor_visible = !cursor_visible;
            gotoxy(cursor_x, cursor_y);
            vga_put_char(cursor_visible ? '_' : ' ');
            gotoxy(cursor_x, cursor_y);
            last_blink = timer_ticks;
        }
        
        if (key_pressed) {
            keycode_t code = last_keycode;
            key_pressed = 0;
            
            if (code == KEY_ENTER) {
                input_buffer[input_len] = '\\0';
                break;
            } else if (code == KEY_BACKSPACE && input_len > 0) {
                input_len--;
                gotoxy(cursor_x - 1, cursor_y);
                vga_put_char(' ');
                gotoxy(cursor_x - 1, cursor_y);
            } else if (input_len < INPUT_BUFFER_SIZE - 1 && keycode_to_ascii[code]) {
                input_buffer[input_len++] = keycode_to_ascii[code];
                vga_put_char(input_buffer[input_len - 1]);
            }
        }
    }
    
    /* Parse input based on format */
    if (format[0] == '%' && format[1] == 's') {
        char* str = va_arg(args, char*);
        strcpy(str, input_buffer);
    } else if (format[0] == '%' && (format[1] == 'd' || format[1] == 'l')) {
        long* num = va_arg(args, long*);
        *num = 0;
        for (size_t i = 0; i < input_len; i++) {
            if (input_buffer[i] >= '0' && input_buffer[i] <= '9') {
                *num = *num * 10 + (input_buffer[i] - '0');
            }
        }
    }
    
    va_end(args);
}"

# Create src/drivers/timer.h
create_file "$BASE_DIR/src/drivers/timer.h" \
"#ifndef TIMER_H
#define TIMER_H

#define BLINK_SPEED 50 /* 2 Hz at 100 Hz timer */

extern volatile uint32_t timer_ticks;

void timer_init(uint32_t frequency);
void timer_wait(uint32_t ticks);

#endif"

# Create src/drivers/timer.c
create_file "$BASE_DIR/src/drivers/timer.c" \
"#include \"timer.h\"
#include \"interrupt/idt.h\"
#include <stdint.h>

volatile uint32_t timer_ticks = 0;

/* Initialize PIT (Programmable Interval Timer) */
void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36); /* Channel 0, square wave */
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    outb(0x21, inb(0x21) & ~0x01); /* Enable IRQ0 */
}

/* Timer interrupt handler */
void timer_handler(void) {
    timer_ticks++;
    outb(0x20, 0x20); /* EOI */
}

/* Wait for specified ticks */
void timer_wait(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while (timer_ticks - start < ticks) asm(\"hlt\");
}"

# Create src/lib/printf.h
create_file "$BASE_DIR/src/lib/printf.h" \
"#ifndef PRINTF_H
#define PRINTF_H

int printf(const char* format, ...);

#endif"

# Create src/lib/printf.c
create_file "$BASE_DIR/src/lib/printf.c" \
"#include \"printf.h\"
#include \"vga.h\"
#include \"string.h\"
#include <stdarg.h>
#include <stdint.h>

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[32];
    
    for (const char* p = format; *p; p++) {
        if (*p != '%') {
            vga_put_char(*p);
            continue;
        }
        p++;
        switch (*p) {
            case 's': {
                const char* str = va_arg(args, const char*);
                while (*str) vga_put_char(*str++);
                break;
            }
            case 'c': {
                char c = va_arg(args, int);
                vga_put_char(c);
                break;
            }
            case 'd': {
                int32_t num = va_arg(args, int32_t);
                itoa(num, buffer, 10);
                for (char* s = buffer; *s; s++) vga_put_char(*s);
                break;
            }
            case 'l': {
                int64_t num = va_arg(args, int64_t);
                ltoa(num, buffer, 10);
                for (char* s = buffer; *s; s++) vga_put_char(*s);
                break;
            }
            case 'x': {
                uint32_t num = va_arg(args, uint32_t);
                itoa(num, buffer, 16);
                for (int i = 0; i < 8 - strlen(buffer); i++) vga_put_char('0');
                for (char* s = buffer; *s; s++) vga_put_char(*s);
                break;
            }
            default:
                vga_put_char('%');
                vga_put_char(*p);
        }
    }
    
    va_end(args);
    return 0;
}"

# Create src/lib/scanf.h
create_file "$BASE_DIR/src/lib/scanf.h" \
"#ifndef SCANF_H
#define SCANF_H

int scanf(const char* format, ...);

#endif"

# Create src/lib/scanf.c
create_file "$BASE_DIR/src/lib/scanf.c" \
"#include \"scanf.h\"
#include \"keyboard.h\"
#include <stdarg.h>

/* Wrapper for keyboard.c's scanf */
int scanf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    scanf(format, args); /* Call scanf from keyboard.c */
    va_end(args);
    return 0;
}"

# Create src/lib/string.h
create_file "$BASE_DIR/src/lib/string.h" \
"#ifndef STRING_H
#define STRING_H

#include <stdint.h>

size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
char* itoa(int32_t value, char* str, int base);
char* ltoa(int64_t value, char* str, int base);

#endif"

# Create src/lib/string.c
create_file "$BASE_DIR/src/lib/string.c" \
"#include \"string.h\"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* orig = dest;
    while ((*dest++ = *src++));
    return orig;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* orig = dest;
    while (n-- && (*dest++ = *src++));
    return orig;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* itoa(int32_t value, char* str, int base) {
    char* ptr = str;
    char* start = str;
    uint32_t uvalue = value;
    if (base == 10 && value < 0) {
        *ptr++ = '-';
        start = ptr;
        uvalue = -value;
    }
    do {
        int digit = uvalue % base;
        *ptr++ = digit < 10 ? '0' + digit : 'A' + digit - 10;
        uvalue /= base;
    } while (uvalue);
    *ptr-- = '\\0';
    while (start < ptr) {
        char temp = *start;
        *start++ = *ptr;
        *ptr-- = temp;
    }
    return str;
}

char* ltoa(int64_t value, char* str, int base) {
    char* ptr = str;
    char* start = str;
    uint64_t uvalue = value;
    if (base == 10 && value < 0) {
        *ptr++ = '-';
        start = ptr;
        uvalue = -value;
    }
    do {
        int digit = uvalue % base;
        *ptr++ = digit < 10 ? '0' + digit : 'A' + digit - 10;
        uvalue /= base;
    } while (uvalue);
    *ptr-- = '\\0';
    while (start < ptr) {
        char temp = *start;
        *start++ = *ptr;
        *ptr-- = temp;
    }
    return str;
}"

# Create src/utils/progress.h
create_file "$BASE_DIR/src/utils/progress.h" \
"#ifndef PROGRESS_H
#define PROGRESS_H

void init_progress(uint32_t max_value, uint8_t width);
void update_progress(uint32_t value);
uint32_t get_progress(void);

#endif"

# Create src/utils/progress.c
create_file "$BASE_DIR/src/utils/progress.c" \
"#include \"progress.h\"
#include \"vga.h\"
#include \"printf.h\"

static uint32_t progress_max = 0;
static uint32_t progress_current = 0;
static uint8_t progress_width = 0;
static uint8_t progress_x = 0;
static uint8_t progress_y = 0;

/* Initialize progress bar */
void init_progress(uint32_t max_value, uint8_t width) {
    progress_max = max_value;
    progress_current = 0;
    progress_width = width;
    progress_x = (VGA_WIDTH - width) / 2; /* Center bar */
    progress_y = VGA_HEIGHT - 3; /* Place above action bar */
    update_progress(0);
}

/* Update progress bar */
void update_progress(uint32_t value) {
    progress_current = value > progress_max ? progress_max : value;
    float ratio = (float)progress_current / progress_max;
    uint32_t filled = (uint32_t)(ratio * progress_width);
    uint32_t empty = progress_width - filled;
    
    gotoxy(progress_x, progress_y);
    set_textcolor(COLOR_DARK_GREY, COLOR_WHITE);
    for (uint32_t i = 0; i < filled; i++) vga_put_char(' ');
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    for (uint32_t i = 0; i < empty; i++) vga_put_char(' ');
    
    gotoxy(progress_x + progress_width + 2, progress_y);
    printf(\"%d%%\", (int)(ratio * 100));
}

/* Get current progress */
uint32_t get_progress(void) {
    return progress_current;
}"

# Create src/utils/reboot.h
create_file "$BASE_DIR/src/utils/reboot.h" \
"#ifndef REBOOT_H
#define REBOOT_H

void reboot(void);
void reboot_acpi(void);
void reboot_kbd(void);
void reboot_triple_fault(void);

#endif"

# Create src/utils/reboot.c
create_file "$BASE_DIR/src/utils/reboot.c" \
"#include \"reboot.h\"
#include <stdint.h>

/* Reboot via keyboard controller (default) */
void reboot_kbd(void) {
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 0x01) inb(0x60); /* Clear buffer */
    } while (temp & 0x02); /* Wait for input buffer empty */
    outb(0x64, 0xFE); /* Pulse reset line */
    for(;;) asm(\"hlt\");
}

/* Reboot via ACPI (placeholder, requires ACPI tables) */
void reboot_acpi(void) {
    /* TODO: Implement ACPI reset */
    reboot_kbd();
}

/* Reboot via triple fault */
void reboot_triple_fault(void) {
    asm volatile(\"lidt [0]\");
    asm volatile(\"int $0\");
    for(;;) asm(\"hlt\");
}

/* Default reboot method */
void reboot(void) {
    reboot_kbd();
}"

# Create src/interrupt/idt.h
create_file "$BASE_DIR/src/interrupt/idt.h" \
"#ifndef IDT_H
#define IDT_H

#include <stdint.h>

void idt_init(void);
void keyboard_handler(void);
void timer_handler(void);

#endif"

# Create src/interrupt/idt.c
create_file "$BASE_DIR/src/interrupt/idt.c" \
"#include \"idt.h\"
#include \"keyboard.h\"
#include \"timer.h\"

#define IDT_SIZE 256
#define GATE_INT 0x8E /* Interrupt gate, 32-bit, present */

static struct {
    uint16_t low;
    uint16_t sel;
    uint8_t zero;
    uint8_t flags;
    uint16_t high;
} idt[IDT_SIZE];

static struct {
    uint16_t limit;
    uint32_t base;
} idt_ptr;

/* Set IDT entry */
static void set_gate(uint8_t n, uint32_t handler) {
    idt[n].low = handler & 0xFFFF;
    idt[n].sel = 0x08; /* Kernel code segment */
    idt[n].zero = 0;
    idt[n].flags = GATE_INT;
    idt[n].high = (handler >> 16) & 0xFFFF;
}

/* Install IDT */
void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    /* Clear IDT */
    for (size_t i = 0; i < IDT_SIZE; i++) {
        set_gate(i, (uint32_t)0);
    }
    
    /* Set handlers */
    set_gate(0x20, (uint32_t)timer_handler);
    set_gate(0x21, (uint32_t)keyboard_handler);
    
    /* Load IDT */
    asm volatile(\"lidt %0\" : : \"m\"(idt_ptr));
    
    /* Enable interrupts */
    asm volatile(\"sti\");
}"

# Create src/pages/page.h
create_file "$BASE_DIR/src/pages/page.h" \
"#ifndef PAGE_H
#define PAGE_H

#include \"../drivers/keyboard.h\"

typedef struct {
    const char* keyname;
    keycode_t keycode;
    const char* actionname;
    void (*action)(void);
} action_t;

void display_action_bar(action_t* actions, size_t num_actions);
void handle_input(action_t* actions, size_t num_actions);

#endif"

# Create src/pages/page.c
create_file "$BASE_DIR/src/pages/page.c" \
"#include \"page.h\"
#include \"vga.h\"
#include \"printf.h\"

void display_action_bar(action_t* actions, size_t num_actions) {
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, VGA_HEIGHT - 1);
    for (size_t i = 0; i < VGA_WIDTH; i++) vga_put_char(' ');
    gotoxy(2, VGA_HEIGHT - 1);
    for (size_t i = 0; i < num_actions; i++) {
        printf(\"%s=%s  \", actions[i].keyname, actions[i].actionname);
    }
}

void handle_input(action_t* actions, size_t num_actions) {
    while (1) {
        if (key_pressed) {
            keycode_t code = last_keycode;
            key_pressed = 0;
            for (size_t i = 0; i < num_actions; i++) {
                if (code == actions[i].keycode) {
                    actions[i].action();
                    return;
                }
            }
        }
        asm(\"hlt\");
    }
}"

# Create src/pages/welcome/welcome_page.h
create_file "$BASE_DIR/src/pages/welcome/welcome_page.h" \
"#ifndef WELCOME_PAGE_H
#define WELCOME_PAGE_H

void welcome_page(void);

#endif"

# Create src/pages/welcome/welcome_page.c
create_file "$BASE_DIR/src/pages/welcome/welcome_page.c" \
"#include \"welcome_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../lib/printf.h\"
#include \"../../utils/reboot.h\"
#include \"../tos/tos_page.h\"
#include \"../../main.h\"

static void proceed_action(void) {
    tos_page();
}

static void reboot_action(void) {
    reboot();
}

static action_t actions[] = {
    {\"Enter\", KEY_ENTER, \"Proceed\", proceed_action},
    {\"Esc\", KEY_ESC, \"Reboot\", reboot_action}
};

void welcome_page(void) {
    clrscr();
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, 0);
    printf(\"Welcome to the %s Installation Wizard\\n\", OS_NAME);
    printf(\"--------------------------------------\\n\");
    printf(\"Thank you for choosing %s, a free and open-source operating system.\\n\", OS_NAME);
    printf(\"This wizard will guide you through the installation process.\\n\\n\");
    printf(\"Press Enter to begin or Esc to reboot your computer.\\n\");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}"

# Create src/pages/tos/tos_page.h
create_file "$BASE_DIR/src/pages/tos/tos_page.h" \
"#ifndef TOS_PAGE_H
#define TOS_PAGE_H

void tos_page(void);

#endif"

# Create src/pages/tos/tos_page.c
create_file "$BASE_DIR/src/pages/tos/tos_page.c" \
"#include \"tos_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../lib/printf.h\"
#include \"../../utils/reboot.h\"
#include \"../setup/setup_page.h\"
#include \"../../main.h\"

static void agree_action(void) {
    setup_page();
}

static void disagree_action(void) {
    reboot();
}

static action_t actions[] = {
    {\"F8\", KEY_F8, \" Agree\", agree_action},
    {\"Esc\", KEY_ESC, \"Not Agree\", disagree_action}
};

void tos_page(void) {
    clrscr();
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, 0);
    printf(\"%s Terms of Service\\n\", OS_NAME);
    printf(\"---------------------\\n\");
    printf(\"%s is free software distributed under an open-source license.\\n\", OS_NAME);
    printf(\"It comes with no warranty, express or implied. By installing %s,\\n\", OS_NAME);
    printf(\"you agree to use it for lawful purposes only and accept all risks\\n\");
    printf(\"associated with its use.\\n\\n\");
    printf(\"Please read and agree to proceed with the installation.\\n\");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}"

# Create src/pages/setup/setup_page.h
create_file "$BASE_DIR/src/pages/setup/setup_page.h" \
"#ifndef SETUP_PAGE_H
#define SETUP_PAGE_H

void setup_page(void);

#endif"

# Create src/pages/setup/setup_page.c
create_file "$BASE_DIR/src/pages/setup/setup_page.c" \
"#include \"setup_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../drivers/keyboard.h\"
#include \"../../lib/printf.h\"
#include \"../../utils/reboot.h\"
#include \"../create_partition/create_partition_page.h\"
#include \"../delete_partition/delete_partition_page.h\"
#include \"../format_partition/format_partition_page.h\"
#include \"../installing/installing_page.h\"
#include \"../../main.h\"

typedef struct {
    uint32_t size_mb; /* Size in MB */
    const char* format; /* e.g., \"FAT32\" or \"Unformatted\" */
} partition_t;

typedef struct {
    uint32_t total_size_mb;
    partition_t* partitions;
    size_t num_partitions;
    uint32_t free_space_mb;
} disk_t;

static disk_t disks[] = {
    {1024, (partition_t[]){{500, \"FAT32\"}, {300, \"Unformatted\"}}, 2, 224},
    {2048, NULL, 0, 2048}
};

static size_t selected_disk = 0;
static size_t selected_item = 0; /* 0 to num_partitions for partitions, num_partitions for free space */
static int is_free_space = 0;

static void install_action(void) {
    installing_page();
}

static void create_partition_action(void) {
    create_partition_page();
}

static void delete_partition_action(void) {
    delete_partition_page();
}

static void format_partition_action(void) {
    format_partition_page();
}

static void quit_action(void) {
    reboot();
}

static action_t partition_actions[] = {
    {\"Enter\", KEY_ENTER, \"Install\", install_action},
    {\"C\", KEY_C, \"Create Partition\", create_partition_action},
    {\"D\", KEY_D, \"Delete Partition\", delete_partition_action},
    {\"F\", KEY_F, \"Format Partition\", format_partition_action},
    {\"F3\", KEY_F3, \"Quit\", quit_action}
};

static action_t free_space_actions[] = {
    {\"Enter\", KEY_ENTER, \"Install\", install_action},
    {\"C\", KEY_C, \"Create Partition\", create_partition_action},
    {\"F3\", KEY_F3, \"Quit\", quit_action}
};

static void display_disks(void) {
    gotoxy(0, 3);
    size_t line = 3;
    size_t item_idx = 0;
    
    for (size_t i = 0; i < sizeof(disks) / sizeof(disks[0]); i++) {
        printf(\"Disk %d (%dMB):\\n\", i, disks[i].total_size_mb);
        line++;
        
        for (size_t j = 0; j < disks[i].num_partitions; j++) {
            if (i == selected_disk && item_idx == selected_item && !is_free_space) {
                set_textcolor(COLOR_WHITE, COLOR_BLACK);
            } else {
                set_textcolor(COLOR_BLACK, COLOR_WHITE);
            }
            printf(\"  - Partition %d: %dMB (%s)\\n\", j + 1, disks[i].partitions[j].size_mb,
                   disks[i].partitions[j].format);
            set_textcolor(COLOR_BLACK, COLOR_WHITE);
            line++;
            item_idx++;
        }
        
        if (disks[i].free_space_mb > 0) {
            if (i == selected_disk && item_idx == selected_item && is_free_space) {
                set_textcolor(COLOR_WHITE, COLOR_BLACK);
            } else {
                set_textcolor(COLOR_BLACK, COLOR_WHITE);
            }
            printf(\"  - Free Space: %dMB\\n\", disks[i].free_space_mb);
            set_textcolor(COLOR_BLACK, COLOR_WHITE);
            line++;
            item_idx++;
        }
    }
}

void setup_page(void) {
    while (1) {
        clrscr();
        set_textcolor(COLOR_BLACK, COLOR_WHITE);
        gotoxy(0, 0);
        printf(\"Select a Partition for %s Installation\\n\", OS_NAME);
        printf(\"---------------------------------------\\n\");
        printf(\"Please select a storage device and partition to install %s.\\n\", OS_NAME);
        
        display_disks();
        
        action_t* actions = is_free_space ? free_space_actions : partition_actions;
        size_t num_actions = is_free_space ? sizeof(free_space_actions) / sizeof(free_space_actions[0])
                                          : sizeof(partition_actions) / sizeof(partition_actions[0]);
        display_action_bar(actions, num_actions);
        
        while (1) {
            if (key_pressed) {
                keycode_t code = last_keycode;
                key_pressed = 0;
                
                if (code == KEY_UP) {
                    if (selected_item > 0) {
                        selected_item--;
                        if (selected_item < disks[selected_disk].num_partitions) {
                            is_free_space = 0;
                        } else {
                            is_free_space = 1;
                        }
                        break;
                    }
                } else if (code == KEY_DOWN) {
                    size_t max_item = disks[selected_disk].num_partitions + (disks[selected_disk].free_space_mb > 0 ? 1 : 0);
                    if (selected_item + 1 < max_item) {
                        selected_item++;
                        if (selected_item >= disks[selected_disk].num_partitions) {
                            is_free_space = 1;
                        } else {
                            is_free_space = 0;
                        }
                        break;
                    }
                } else {
                    for (size_t i = 0; i < num_actions; i++) {
                        if (code == actions[i].keycode) {
                            actions[i].action();
                            return;
                        }
                    }
                }
            }
            asm(\"hlt\");
        }
    }
}"

# Create src/pages/create_partition/create_partition_page.h
create_file "$BASE_DIR/src/pages/create_partition/create_partition_page.h" \
"#ifndef CREATE_PARTITION_PAGE_H
#define CREATE_PARTITION_PAGE_H

void create_partition_page(void);

#endif"

# Create src/pages/create_partition/create_partition_page.c
create_file "$BASE_DIR/src/pages/create_partition/create_partition_page.c" \
"#include \"create_partition_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../lib/printf.h\"
#include \"../../lib/scanf.h\"
#include \"../setup/setup_page.h\"
#include \"../../main.h\"

static void create_action(void) {
    long size_mb;
    gotoxy(10, 6);
    scanf(\"%ld\", &size_mb);
    /* Placeholder: Create partition logic */
    setup_page();
}

static void cancel_action(void) {
    setup_page();
}

static action_t actions[] = {
    {\"Enter\", KEY_ENTER, \"Create\", create_action},
    {\"Esc\", KEY_ESC, \"Cancel\", cancel_action}
};

void create_partition_page(void) {
    clrscr();
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, 0);
    printf(\"Create a New Partition\\n\");
    printf(\"---------------------\\n\");
    printf(\"Selected Device: Disk %d\\n\", selected_disk);
    printf(\"Free Space: %dMB\\n\\n\", disks[selected_disk].free_space_mb);
    printf(\"Enter the size (in MB) for the new partition (max %dMB).\\n\", disks[selected_disk].free_space_mb);
    printf(\"The partition will be created on the selected device.\\n\\n\");
    printf(\"Size (MB): [    ]\\n\");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}"

# Create src/pages/delete_partition/delete_partition_page.h
create_file "$BASE_DIR/src/pages/delete_partition/delete_partition_page.h" \
"#ifndef DELETE_PARTITION_PAGE_H
#define DELETE_PARTITION_PAGE_H

void delete_partition_page(void);

#endif"

# Create src/pages/delete_partition/delete_partition_page.c
create_file "$BASE_DIR/src/pages/delete_partition/delete_partition_page.c" \
"#include \"delete_partition_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../lib/printf.h\"
#include \"../setup/setup_page.h\"
#include \"../../main.h\"

static void remove_action(void) {
    /* Placeholder: Delete partition logic */
    setup_page();
}

static void cancel_action(void) {
    setup_page();
}

static action_t actions[] = {
    {\"D\", KEY_D, \"Remove\", remove_action},
    {\"Esc\", KEY_ESC, \"Cancel\", cancel_action}
};

void delete_partition_page(void) {
    clrscr();
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, 0);
    printf(\"Delete Partition\\n\");
    printf(\"----------------\\n\");
    printf(\"Selected Partition: Disk %d, Partition %d (%dMB, %s)\\n\",
           selected_disk, selected_item + 1, disks[selected_disk].partitions[selected_item].size_mb,
           disks[selected_disk].partitions[selected_item].format);
    printf(\"\\nWARNING: Deleting this partition will erase all data on it.\\n\");
    printf(\"This action cannot be undone. Proceed with caution.\\n\\n\");
    printf(\"Confirm deletion or cancel to return.\\n\");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}"

# Create src/pages/format_partition/format_partition_page.h
create_file "$BASE_DIR/src/pages/format_partition/format_partition_page.h" \
"#ifndef FORMAT_PARTITION_PAGE_H
#define FORMAT_PARTITION_PAGE_H

void format_partition_page(void);

#endif"

# Create src/pages/format_partition/format_partition_page.c
create_file "$BASE_DIR/src/pages/format_partition/format_partition_page.c" \
"#include \"format_partition_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../lib/printf.h\"
#include \"../setup/setup_page.h\"
#include \"../../main.h\"

static void format_action(void) {
    /* Placeholder: Format partition logic */
    setup_page();
}

static void cancel_action(void) {
    setup_page();
}

static action_t actions[] = {
    {\"F\", KEY_F, \"Format\", format_action},
    {\"Esc\", KEY_ESC, \"Cancel\", cancel_action}
};

void format_partition_page(void) {
    clrscr();
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, 0);
    printf(\"Format Partition\\n\");
    printf(\"----------------\\n\");
    printf(\"Selected Partition: Disk %d, Partition %d (%dMB)\\n\",
           selected_disk, selected_item + 1, disks[selected_disk].partitions[selected_item].size_mb);
    printf(\"\\nWARNING: Formatting this partition as FAT32 will erase all data.\\n\");
    printf(\"This action cannot be undone. Ensure you have backups.\\n\\n\");
    printf(\"Confirm formatting or cancel to return.\\n\");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}"

# Create src/pages/installing/installing_page.h
create_file "$BASE_DIR/src/pages/installing/installing_page.h" \
"#ifndef INSTALLING_PAGE_H
#define INSTALLING_PAGE_H

void installing_page(void);
void install_os(void);

#endif"

# Create src/pages/installing/installing_page.c
create_file "$BASE_DIR/src/pages/installing/installing_page.c" \
"#include \"installing_page.h\"
#include \"../page.h\"
#include \"../../drivers/vga.h\"
#include \"../../lib/printf.h\"
#include \"../../utils/progress.h\"
#include \"../../drivers/timer.h\"
#include \"../../main.h\"

void install_os(void) {
    /* Placeholder: Simulate installation */
    for (uint32_t i = 0; i <= 100; i++) {
        update_progress(i);
        timer_wait(10); /* Simulate work */
    }
}

void installing_page(void) {
    clrscr();
    set_textcolor(COLOR_BLACK, COLOR_WHITE);
    gotoxy(0, 0);
    printf(\"Installing %s\\n\", OS_NAME);
    printf(\"---------------\\n\");
    printf(\"Please wait while %s is installed on the selected partition.\\n\\n\", OS_NAME);
    
    init_progress(100, 60);
    uint32_t start_ticks = timer_ticks;
    
    install_os();
    
    /* Display elapsed time */
    uint32_t elapsed = (timer_ticks - start_ticks) / 100; /* Seconds */
    gotoxy(0, VGA_HEIGHT - 2);
    printf(\"Installation complete in %d seconds.\\n\", elapsed);
    
    /* No actions; wait indefinitely */
    for(;;) asm(\"hlt\");
}"

# Set executable permissions for the script
chmod +x "$0"

echo "All files and directories created successfully in $BASE_DIR/"
exit 0