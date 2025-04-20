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
#include <kernel/arch/i386/paging.h>
#include <kernel/arch/i386/mcpuid.h>

#include <kernel/basic_tests.h>
// #include <kernel/mconio.h>
#include <kernel/mport.h>
#include <kernel/process.h>
#include <kernel/fpu.h>
#include <kernel/drivers/vga.h>
#include <kernel/drivers/all.h>

#include <kernel/kernel-base.h>

#include <stdio.h>

static void init_drivers(){
    timer_drv.init();
    keyboard_drv.init();    
}

// Kernel initialization
static void kernel_init(multiboot_info_t* mbi) {
    _hide_cursor();
    stdio_init();
    init_gdt();
    init_paging(mbi);
    // Initialize IDT (sets up exceptions and IRQs)
    idt_init();
    enable_fpu();
    init_drivers();
    init_processes();

    load_multiboot_mods(mbi);

    mouse_drv.init(80,25);
    mouse_drv.set_speed(20); // Faster movement
    mouse_drv.set_resolution(1); // 2 counts/mm
    mouse_drv.set_sample_rate(100); // 100 Hz    
    mouse_drv.disable_mouse();
    mouse_drv.enable_mouse();


    vga_clear();


}

// Kernel main function
void kernel_main(multiboot_info_t* mbi) {
    kernel_init(mbi);

    
    show_processes(0, 10); // Show first 10 processes

    b_test_mouse();
    // b_test_isr_driver();
    // b_test_idt();
    // b_test_mbi(mbi);
    // b_test_mconio();
    // b_test_mconio_scroll();    
    // b_test_a20();
    // b_test_multiboot_header();
    // b_test_cupid();
    // b_test_mconio();
    // // b_test_mconio_scroll();    
    // // b_test_a20();
    // // b_test_multiboot_header();
    // b_test_cupid();


    halt();      // while(1);
}