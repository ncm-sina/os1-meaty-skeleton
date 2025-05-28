#include "../etc/serial.h"
#include <stdio.h>

// // Syscall wrapper
// void syscall_print(const char *msg) {
//     asm volatile (
//         "mov $1, %%eax\n" // Assume syscall 1 is print
//         "mov %0, %%ebx\n"
//         "int $0x80"
//         :
//         : "r"(msg)
//         : "eax", "ebx"
//     );
// }

// int main(int argc, char *argv[]) {
//     syscall_print("Hello from init!\n");
//     for (int i = 0; i < argc; i++) {
//         syscall_print(argv[i]);
//         syscall_print("\n");
//     }
//     // Execute shell
//     asm volatile (
//         "mov $3, %%eax\n" // Assume syscall 3 is execute
//         "mov %0, %%ebx\n"
//         "mov %1, %%ecx\n"
//         "int $0x80"
//         :
//         : "r"("/sbin/shell.elf"), "r"((char *[]){"shell", NULL})
//         : "eax", "ebx", "ecx"
//     );
//     while (1) {} // Keep running
//     return 0;
// }

int main(void) {
    serial_printf("my os init process");
    // printf("MyOS init process\n");
    // TODO: Initialize system services
    while (1);
    return 0;
}
