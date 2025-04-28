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

// multiboot_info_t *_mbi;

static void init_graphics(){
    int32_t res,res2, res3;
    // res = vbe_is_text_mode();
    // res2 = vbe_set_text_mode();
    // res3 = vbe_is_text_mode();
    // printf("aaa: %08x %d | bbb: %08x %d | ccc: %08x %d", res, res, res2, res2, res3, res3);
    if (_mbi->flags & (1 << 11)) {
        if(res2 = vbe_set_text_mode()) printf("1error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("VBE: control=0x%08X, mode=0x%08X", _mbi->vbe_control_info, _mbi->vbe_mode_info);
    }else{
        if(res2 = vbe_set_text_mode()) printf("2error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("f1\n");
    }
    if (_mbi->flags & (1 << 12)) {
        if(res2 = vbe_set_text_mode()) printf("3error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("Framebuffer: addr=0x%11X \n", _mbi->framebuffer_addr);
    }else{
        if(res2 = vbe_set_text_mode()) printf("4error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("f2\n");
    }   
    // while(1);
    if((res = vbe_init()) <0){
        if(res2 = vbe_set_text_mode()) printf("5error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("error init graph can't use os : %08x %d", res, res);
        // return;
    }
    if((res = vbe_set_mode(1024, 768, 32))){
        if(res2 = vbe_set_text_mode()) printf("6error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("error set vbe mode can't use os :%08x %d", res, res);
        // return;
    }
    vbe_clear_screen(0xa0ff00);
    if(res<0){
        if((res2 = vbe_set_text_mode()) < 0) printf("7error setting text mode err:%08x \n",res2);
        printf(" no ");
    }else{       
        // vbe_set_mode(1024,768,32);
        if((res2 = vbe_set_text_mode()) < 0) printf("8error setting text mode err:%08x \n",res2);
        printf(" yes ");
    }
    while(1);

}

static void init_drivers(){
    timer_drv.init();
    keyboard_drv.init();    
    init_graphics();
    while(1);
}

// Kernel initialization
static void kernel_init(multiboot_info_t* _mbi) {
    _mbi = _mbi;
    _hide_cursor();
    stdio_init();
    init_gdt();
    init_paging(_mbi);
    // Initialize IDT (sets up exceptions and IRQs)
    idt_init();
    enable_fpu();
    load_multiboot_mods(_mbi);
    init_drivers();
    init_processes();


    mouse_drv.init(80,25);
    mouse_drv.set_speed(20); // Faster movement
    mouse_drv.set_resolution(1); // 2 counts/mm
    mouse_drv.set_sample_rate(100); // 100 Hz    
    mouse_drv.disable_mouse();
    mouse_drv.enable_mouse();


    vga_clear();


}

// Kernel main function
void kernel_main(multiboot_info_t* _mbi) {
    kernel_init(_mbi);

    
    show_processes(0, 10); // Show first 10 processes

    b_test_mouse();
    // b_test_isr_driver();
    // b_test_idt();
    // b_test_mbi(_mbi);
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