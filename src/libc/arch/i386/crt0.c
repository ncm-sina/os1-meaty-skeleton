// GCC provides these header files automatically
// They give us access to useful things like fixed-width types
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYS_exit           1

// crt0.c
void _start() {
    // Stack: [0, argc, argv[0], argv[1], ..., NULL]
    uint32_t *stack;
    asm volatile ("mov %%esp, %0" : "=r"(stack));
    int argc = stack[1];
    char **argv = (char **)&stack[2];
    // Call main
    int ret = main(argc, argv);
    // Exit syscall using SYS_exit macro
    asm volatile (
        "mov %1, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80"
        :
        : "r"(ret), "i"(SYS_exit)
        : "eax", "ebx"
    );
}