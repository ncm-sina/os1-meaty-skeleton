#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src,  unsigned int n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int stricmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

int snprintf(char *str, unsigned int size, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
