#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Standard streams */
FILE *stdin;
FILE *stdout;
FILE *stderr;

/* Static storage for standard streams */
static FILE stdin_struct;
static FILE stdout_struct;
static FILE stderr_struct;

/* Buffers for stdout */
static char stdout_buffer[BUFSIZ];

 
// TODO: this should be called somewhere probably at crt0.c
void stdio_init(void) {
    /* Initialize stdin (not used for output) */
    stdin_struct.fd = 0;
    stdin_struct.buffer = NULL;
    stdin_struct.buf_size = 0;
    stdin_struct.buf_pos = 0;
    stdin_struct.buf_mode = _IONBF;
    stdin_struct.error = 0;
    stdin = &stdin_struct;

    /* Initialize stdout (line-buffered) */
    stdout_struct.fd = 1;
    stdout_struct.buffer = stdout_buffer;
    stdout_struct.buf_size = BUFSIZ;
    stdout_struct.buf_pos = 0;
    stdout_struct.buf_mode = _IOLBF;
    stdout_struct.error = 0;
    stdout = &stdout_struct;

    /* Initialize stderr (unbuffered) */
    stderr_struct.fd = 2;
    stderr_struct.buffer = NULL;
    stderr_struct.buf_size = 0;
    stderr_struct.buf_pos = 0;
    stderr_struct.buf_mode = _IONBF;
    stderr_struct.error = 0;
    stderr = &stderr_struct;
}