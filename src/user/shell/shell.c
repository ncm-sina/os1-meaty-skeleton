#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

// // Syscall wrappers
// void syscall_print(const char *msg) {
//     asm volatile (
//         "mov $1, %%eax\n"
//         "mov %0, %%ebx\n"
//         "int $0x80"
//         :
//         : "r"(msg)
//         : "eax", "ebx"
//     );
// }

// void syscall_execute(const char *path, char *argv[]) {
//     asm volatile (
//         "mov $3, %%eax\n"
//         "mov %0, %%ebx\n"
//         "mov %1, %%ecx\n"
//         "int $0x80"
//         :
//         : "r"(path), "r"(argv)
//         : "eax", "ebx", "ecx"
//     );
// }

// int main(int argc, char *argv[]) {
//     // Simplified: hardcode a program execution
//     syscall_print("Starting myprog\n");
//     char *args[] = {"myprog", "arg1", "arg2", NULL};
//     syscall_execute("/sbin/myprog.elf", args);
//     syscall_print("Program finished\n");
//     while (1) {} // Keep running
//     return 0;
// }

int main(void) {
    // printf("MyOS Shell\n");
    while (1) {
        // printf("%s ", getenv("prompt"));
        // TODO: Read and execute commands
    }
    return 0;
}
