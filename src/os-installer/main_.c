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

#include <kernel/arch/i386/idt.h>
#include <kernel/arch/i386/isrs/all.h>
// #include <kernel/arch/i386/paging.h>
#include <kernel/arch/i386/mcpuid.h>

// #include <kernel/basic_tests.h>
// #include <kernel/mconio.h>
#include <kernel/mport.h>
#include <kernel/process.h>
#include <kernel/fpu.h>
// #include <kernel/drivers/vga.h>
// #include <kernel/drivers/vbe.h>
#include <kernel/drivers/all.h>

// #include <kernel/utils/bmp.h>

#include <kernel/kernel-base.h>

#include <stdio.h>

#include "pages/welcome/welcome_page.h"


multiboot_info_t *_mbi;

// extern uint32_t* framebuffer;
extern uint64_t framebuffer_addr; // framebuffer physical address or dynamic vaddr (we should map this to a fixed address later)



static void init_drivers(){
    printf(" init drivers ");
    timer_drv.init();
    keyboard_drv.init();    
    // init_graphics();
}


// Kernel initialization
static void kernel_init(multiboot_info_t* mbi) {
    _mbi = mbi;
    _hide_cursor();
    idt_init();           /* Set up IDT for interrupts */
    init_drivers();


}

// Kernel main function
void kernel_main(multiboot_info_t* _mbi) {
    kernel_init(_mbi);
    /* Initialize drivers */
    // b_test_isr_driver();
    
    /* this will take care of the business */
    run_os_installer_wisard();
    
    /* Should not reach here */
    halt();      // while(1);
}