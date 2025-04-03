// GCC provides these header files automatically
// They give us access to useful things like fixed-width types
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// First, let's do some basic checks to make sure we are using our x86-elf cross-compiler correctly
#if defined(__linux__)
	#error "This code must be compiled with a cross-compiler"
#elif !defined(__i386__)
	#error "This code must be compiled with an x86-elf compiler"
#endif

#include <kernel/basic_tests.h>

#include <kernel/mconio.h>
#include <kernel/mport.h>
#include <kernel/vga_basic.h>
#include <kernel/mcpuid.h>

#include <kernel/kernel-base.h>

// Static function to hide the cursor using port I/O
static void _hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
}

// Kernel initialization
static void kernel_init(void) {
    _hide_cursor();
    clrscr();
    

}

// Kernel main function
void kernel_main(void) {
    kernel_init();
    // b_test_mconio();
    // b_test_mconio_scroll();    
    // b_test_a20();
    // b_test_multiboot_header();
    b_test_cupid();

}