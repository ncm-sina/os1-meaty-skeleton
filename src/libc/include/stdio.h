#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <sys/types.h>


#define EOF (-1)

/* Buffering modes */
#define _IONBF 0 /* Unbuffered */
#define _IOLBF 1 /* Line buffered */
#define _IOFBF 2 /* Fully buffered */

/* Stream flags */
#define _IO_READ   0x0001 /* Stream is readable */
#define _IO_WRITE  0x0002 /* Stream is writable */
#define _IO_CLOSED 0x0004 /* Stream is closed */
#define _IO_EOF    0x0008 /* EOF reached */
#define _IO_ERR    0x0010 /* Error occurred */

/* Default buffer size */
#define BUFSIZ 1024

#ifdef __cplusplus
extern "C" {
#endif

/* FILE structure for stdio streams */
typedef struct _FILE {
    int fd;             /* File descriptor */
    int mode;           /* Open mode (O_RDONLY, O_WRONLY, O_RDWR) */
    int flags;          /* Stream flags (_IO_READ, _IO_WRITE, etc.) */
    char *buffer;       /* Buffer for data */
    size_t buf_size;    /* Buffer size */
    size_t buf_pos;     /* Current position in buffer */
    int buf_mode;       /* Buffering mode (_IOFBF, _IOLBF, _IONBF) */
    off_t pos;          /* File position for fseek/ftell */
    int error;          /* Error flag */
    int eof;            /* EOF flag */
} FILE;

// typedef struct FILE FILE;

/* Standard streams */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

void format_string(char *output, const char *fstring, va_list args);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
int fprintf(FILE *stream, const char *format, ...);
int printf(const char *format, ...);
int putchar(char ch);
int vfprintf(FILE *stream, const char *format, va_list args);
int sprintf(char *str, const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int scanf(const char *format, ...);
int getc(FILE *stream);
int putc(int c, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputs(const char *s, FILE *stream);
int write_buffer(FILE *stream, const char *buf, int len);
int fflush(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
