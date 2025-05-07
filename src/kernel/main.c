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
#include <kernel/drivers/vbe.h>
#include <kernel/drivers/all.h>

#include <kernel/utils/bmp.h>

#include <kernel/kernel-base.h>

#include <stdio.h>

multiboot_info_t *_mbi;

// extern uint32_t* framebuffer;
extern uint64_t framebuffer_addr; // framebuffer physical address or dynamic vaddr (we should map this to a fixed address later)


static void init_graphics(){
    int32_t res,res2, res3;
    // res = vbe_is_text_mode();
    // res2 = vbe_set_text_mode();
    // res3 = vbe_is_text_mode();
    // printf("aaa: %08x %d | bbb: %08x %d | ccc: %08x %d", res, res, res2, res2, res3, res3);
    // if (_mbi->flags & (1 << 11)) {
    //     if(res2 = vbe_set_text_mode()) printf("1error setting text mode err:%08x err2: %d \n", res2, res2);
    //     printf("VBE: control=0x%08X, mode=0x%08X", _mbi->vbe_control_info, _mbi->vbe_mode_info);
    // }else{
    //     if(res2 = vbe_set_text_mode()) printf("2error setting text mode err:%08x err2: %d \n", res2, res2);
    //     printf("f1\n");
    // }
    // if (_mbi->flags & (1 << 12)) {
    //     if(res2 = vbe_set_text_mode()) printf("3error setting text mode err:%08x err2: %d \n", res2, res2);
    //     printf("Framebuffer: addr=0x%11X \n", _mbi->framebuffer_addr);
    // }else{
    //     if(res2 = vbe_set_text_mode()) printf("4error setting text mode err:%08x err2: %d \n", res2, res2);
    //     printf("f2\n");
    // }
    // while(1);
    if((res = vbe_init()) <0){
        if(res2 = vbe_set_text_mode()) printf("5error setting text mode err:%08x err2: %d \n", res2, res2);
        printf("error init graph can't use os : %08x %d", res, res);
        return;
    }
    vbe_info_block_t tmp_vbe_info;
    memcpy(&tmp_vbe_info, 0x9000, sizeof(vbe_info_block_t));
    // if(res2 = vbe_set_text_mode()) printf("5error setting text mode err:%08x err2: %d \n", res2, res2);
    // printf("-> init graph res : %08x %d", res, res);
    // print_vbe_info_block(0x9000);

    // print_vbe_info_block(&tmp_vbe_info);

    // uint16_t cmode;
    // vbe_get_current_mode(&cmode);
    // if(res2 = vbe_set_text_mode()) printf("6error setting text mode err:%08x err2: %d \n", res2, res2);
    // printf("cmode: %08x ", cmode);
    // while(1);
    // if((res = vbe_set_mode(1024, 768, 32))){
    //     if(res2 = vbe_set_text_mode()) printf("6error setting text mode err:%08x err2: %d \n", res2, res2);
    //     printf("error set vbe mode can't use os :%08x %d", res, res);
    //     // return;
    // }
    
    vbe_mode_info_t tmp_vbe_minfo;
    // vbe_get_mode_info(0x118);
    memcpy(&tmp_vbe_minfo, 0xA000, sizeof(vbe_mode_info_t));
    // if(res2 = vbe_set_text_mode()) printf("5error setting text mode err:%08x err2: %d \n", res2, res2);
    // print_vbe_info_block(&tmp_vbe_info);
    // print_vbe_mode_info(&tmp_vbe_minfo);
    // printf("\n ----> framebuffer:%08x ", _mbi->framebuffer_addr);

    // print_mmap_addresses(_mbi);
    // print_module_info(_mbi);

    // vbe_list_supported_modes();
    // while(1);
    
    // vbe_clear_screen(0x2233cc/*, &tmp_vbe_minfo*/);
    // while(1);
    // if(res<0){
    //     if((res2 = vbe_set_text_mode()) < 0) printf("7error setting text mode err:%08x \n",res2);
    //     printf(" no ");
    // }else{
    //     // vbe_set_mode(1024,768,32);
    //     if((res2 = vbe_set_text_mode()) < 0) printf("8error setting text mode err:%08x \n",res2);
    //     printf(" yes ");
    // }
    // while(1);

}

static void init_drivers(){
    printf(" init drivers ");
    timer_drv.init();
    keyboard_drv.init();    
    init_graphics();
}

// Example usage in your kernel
void display_image(const uint8_t *bmp_buffer, uint32_t buffer_size) {
    Pixel32 *pixel_buffer = (Pixel32 *)0xD0000000; // Example address for output buffer
    uint32_t width, height;

    int result = read_bmp(bmp_buffer, buffer_size, pixel_buffer, &width, &height);
    if (result != 0) {
        // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        // printf("res:%d ", result);
        // while(1);    
        // Handle error (e.g., print to serial console)
        return;
    }

    // Draw pixels using your vbe_draw_pixel function
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Pixel32 pixel = pixel_buffer[y * width + x];
            uint32_t color = (pixel.red << 16) | (pixel.green << 8) | pixel.blue;
            vbe_draw_pixel(x, y, color); // Assuming vbe_draw_pixel takes RGB
        }
    }
}

static void draw_background(multiboot_info_t* mbi){
    multiboot_module_t *mod = 0;
    mod = get_multiboot_mod_by_name(mbi, "the-skeleton.bmp");
    if (mod->mod_start >= mod->mod_end) {
        printf("Invalid module\n");
        return;
    }
    
    display_image( mod->mod_start, mod->mod_end - mod->mod_start );    
    
}

// Kernel initialization
static void kernel_init(multiboot_info_t* mbi) {
    _mbi = mbi;
    _hide_cursor();
    // stdio_init();
    // init_gdt();
    init_paging_stage2(_mbi);
    // Initialize IDT (sets up exceptions and IRQs)
    idt_init();
    enable_fpu();
    load_multiboot_mods(_mbi);
    init_drivers();
    init_processes();

    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // draw_background(mbi);
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    vbe_set_fg_color(0xff,0x0, 0x0);
    printf2("01234567890123456789012345678901234567890123456789");
    vbe_set_fg_color(0xff,0xff, 0x0);
    printf2("01234567890123456789012345678901234567890123456789");
    vbe_set_fg_color(0x0, 0xff, 0x0);
    printf2("01234567890123456789012345678901234567890123456789");
    vbe_set_fg_color(0x0, 0xff, 0xff);
    printf2("0");

    // vbe_clear_screen(0xFFFF00/*, &tmp_vbe_minfo*/);
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // print_multiboot_info(_mbi);
    // printf("fb_addr: %08x width: %d height: %d pitch: %d bpp: %d ", framebuffer.fb_addr, framebuffer.width, framebuffer.height, framebuffer.pitch, framebuffer.bpp);
    while(1);
    
    
    mouse_drv.init(1024,768);
    mouse_drv.set_speed(20); // Faster movement
    mouse_drv.set_resolution(1); // 2 counts/mm
    mouse_drv.set_sample_rate(100); // 100 Hz    
    mouse_drv.disable_mouse();
    mouse_drv.enable_mouse();
    while(1);


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