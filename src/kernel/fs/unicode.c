#include <kernel/fs/unicode.h>

// External console output function (replace with your OS's equivalent)
extern void console_putc(char c);

// void unicode_print(const uint16_t* str) {
//     // Convert to ASCII and print character by character
//     while (*str != 0x0000) {
//         if (*str <= 0x007F) {
//             console_putc((char)*str); // ASCII character
//         } else {
//             console_putc('?'); // Non-ASCII replaced with '?'
//         }
//         str++;
//     }
// }

size_t unicode_strlen(const uint16_t* str) {
    size_t len = 0;
    while (*str++ != 0x0000) {
        len++;
    }
    return len;
}

void unicode_strcpy(uint16_t* dest, const uint16_t* src) {
    while (*src != 0x0000) {
        *dest++ = *src++;
    }
    *dest = 0x0000; // Null terminate
}

int ascii_to_unicode(const char* src, uint16_t* dest, size_t dest_size) {
    size_t i = 0;
    // Check if dest has enough space (including null terminator)
    while (src[i] != '\0') {
        if (i + 1 >= dest_size) {
            return -1; // Destination buffer too small
        }
        dest[i] = (uint16_t)(unsigned char)src[i]; // Zero-extend ASCII to UTF-16LE
        i++;
    }
    if (i + 1 >= dest_size) {
        return -1; // No space for null terminator
    }
    dest[i] = 0x0000; // Null terminate
    return 0;
}

int unicode_to_ascii(const uint16_t* src, char* dest, size_t dest_size) {
    size_t i = 0;
    // Convert until null terminator
    while (src[i] != 0x0000) {
        if (i + 1 > dest_size) {
            return -1; // Destination buffer too small
        }
        dest[i] = (src[i] <= 0x007F) ? (char)src[i] : '?'; // ASCII or '?'
        i++;
    }
    if (i > dest_size) {
        return -1; // No space for null terminator
    }
    dest[i] = '\0'; // Null terminate
    return 0;
}