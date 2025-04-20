#ifndef _CTYPE_H
#define _CTYPE_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);
int isprint(int c);
int tolower(int c);
int toupper(int c);

#ifdef __cplusplus
}
#endif

#endif
