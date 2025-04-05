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
#include <kernel/paging.h>

#include <kernel/kernel-base.h>

// Static function to hide the cursor using port I/O
static void _hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
}

// Kernel initialization
static void kernel_init(/*multiboot_info_t* mbi*/) {
    _hide_cursor();
    clrscr();
    // init_paging(mbi);    

}

// Kernel main function
void kernel_main(multiboot_info_t* mbi) {
    kernel_init(/*mbi*/);
    cprintf("flags=%08X, mem_lower=%08X, mem_upper=%08X, boot_device=%08X \n", mbi->flags, mbi->mem_lower, mbi->mem_upper, mbi->boot_device);
    cprintf("cmdline=%08X, mods_count=%08X, mods_addr=%08X, syms=%08X \n", mbi->cmdline, mbi->mods_count, mbi->mods_addr, mbi->syms);
    cprintf("mmap_length=%08X, mmap_addr=%08X \n", mbi->mmap_length, mbi->mmap_addr);
    cprintf("\n");
    b_test_mconio();
    // b_test_mconio_scroll();    
    // b_test_a20();
    // b_test_multiboot_header();
    b_test_cupid();
    halt();      // while(1);
}