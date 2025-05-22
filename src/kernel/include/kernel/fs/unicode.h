#ifndef UNICODE_H
#define UNICODE_H

#include <stdint.h>
#include <stddef.h>

// Print a UTF-16LE string to the console (converts to ASCII for display)
// void unicode_print(const uint16_t* str);

// Calculate the length of a UTF-16LE string (excluding null terminator)
size_t unicode_strlen(const uint16_t* str);

// Copy a UTF-16LE string to a destination buffer
void unicode_strcpy(uint16_t* dest, const uint16_t* src);

// Convert an ASCII string to UTF-16LE
// Returns 0 on success, -1 if dest is too small
int ascii_to_unicode(const char* src, uint16_t* dest, size_t dest_size);

// Convert a UTF-16LE string to ASCII (non-ASCII characters become '?')
// Returns 0 on success, -1 if dest is too small
int unicode_to_ascii(const uint16_t* src, char* dest, size_t dest_size);

#endif // UNICODE_H