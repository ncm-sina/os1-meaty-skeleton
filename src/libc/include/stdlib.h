#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

// __attribute__((__noreturn__))

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
int atoi(const char *nptr);
long atol(const char *nptr);
double strtod(const char *nptr, char **endptr);
long strtol(const char *nptr, char **endptr, int base);
void exit(int status);
void abort(void);
int system(const char *command);
char *getenv(const char *name);

#ifdef __cplusplus
}
#endif

#endif
