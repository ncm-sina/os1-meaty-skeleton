#include <string.h>

// Convert character to lowercase (ASCII only)
static int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A'); // Add 32 to convert to lowercase
    }
    return c;
}

// Convert character to uppercase (ASCII only)
static int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A'); // Subtract 32 to convert to uppercase
    }
    return c;
}

int stricmp(const char *s1, const char *s2) {
    while (*s1 && (tolower(*(unsigned char *)s1) == tolower(*(unsigned char *)s2))) {
        s1++;
        s2++;
    }
    return tolower(*(unsigned char *)s1) - tolower(*(unsigned char *)s2);
}
