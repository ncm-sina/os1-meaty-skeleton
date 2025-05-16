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
#include <kernel/drivers/ide.h>
#include <kernel/drivers/serial.h>
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
        // return;
    }

    // vbe_info_block_t tmp_vbe_info;
    // memcpy(&tmp_vbe_info, 0x9000, sizeof(vbe_info_block_t));
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
    
    // vbe_mode_info_t tmp_vbe_minfo;
    // vbe_get_mode_info(0x118);
    // memcpy(&tmp_vbe_minfo, 0xA000, sizeof(vbe_mode_info_t));
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
    idt_init();
    serial_init();
    timer_drv.init();
    ide_init();
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

void test_ide_write(void) {
    struct block_dev *dev = ide_get_block_dev();
    if (!dev) {
        serial_printf("No IDE device\n");
        return;
    }

    // Use a high LBA to avoid critical data (adjust based on VHD size)
    uint32_t lba = 40000; // Safe for a 512MB VHD
    uint8_t write_buffer[512]={0};
    uint8_t read_buffer[512]={0};
    uint8_t original_buffer[512]={0};

    // Initialize write buffer with test pattern
    for (int i = 0; i < 512; i++) {
        write_buffer[i] = 0xAA; // Distinct pattern
    }

    // Read original data to check if sector is unused
    if (dev->read(lba, original_buffer, 1) != 0) {
        serial_printf("Failed to read LBA %d\n", lba);
        return;
    }

    // Check if sector is unused (e.g., all zeros or random)
    int is_unused = 1;
    for (int i = 0; i < 512; i++) {
        if (original_buffer[i] != 0) {
            serial_printf(" %d:%x ", i, original_buffer[i]);
            is_unused = 0; // Sector may contain data
            // break;
        }
    }
    if (!is_unused) {
        serial_printf("Warning: LBA %d contains data. Choose another LBA.\n", lba);
        // return;
    }

    // Write test data
    if (dev->write(lba, write_buffer, 1) != 0) {
        serial_printf("Write to LBA %d failed\n", lba);
        return;
    }

    // Read back to verify
    if (dev->read(lba, read_buffer, 1) != 0) {
        serial_printf("Read-back from LBA %d failed\n", lba);
        return;
    }

    // Verify data
    int match = 1;
    for (int i = 0; i < 512; i++) {
        if (read_buffer[i] != write_buffer[i]) {
            match = 0;
            break;
        }
    }

    if (match) {
        serial_printf("Write test passed: LBA %d written and verified\n", lba);
    } else {
        serial_printf("Write test failed: Data mismatch at LBA %d\n", lba);
    }

    // // Optional: Restore original data (if needed)
    // if (dev->write(lba, original_buffer, 1) != 0) {
    //     serial_printf("Failed to restore LBA %d\n", lba);
    // }
}

void test_mbr_parse(void) {
    struct block_dev *dev = ide_get_block_dev();
    if (!dev) {
        serial_printf("No IDE device\n");
        return;
    }

    uint32_t fat32_lba, fat32_size;
    if (mbr_parse(dev, &fat32_lba, &fat32_size) == 0) {
        serial_printf("FAT32 partition: LBA %u, Size %u sectors\n", fat32_lba, fat32_size);
    } else {
        serial_printf("Failed to parse MBR or find FAT32 partition\n");
    }
}

// Test function for serial_printf
void serial_printf_test(void) {
    int num1;
    unsigned int unum1;
    char *str;
    void *ptr;
    char ch;

    // Test 1: Basic %d (positive)
    num1 = 1000;
    serial_printf("test 1 - [1000]:[%d]\n", num1);

    // Test 2: Basic %x (hex)
    num1 = 16;
    serial_printf("test 2 - [10]:[%x]\n", num1);

    // Test 3: Basic %c
    ch = 'A';
    serial_printf("test 3 - [A]:[%c]\n", ch);

    // Test 4: Basic %s
    str = "hello";
    serial_printf("test 4 - [hello]:[%s]\n", str);

    // Test 5: Basic %u (unsigned)
    unum1 = 1234;
    serial_printf("test 5 - [1234]:[%u]\n", unum1);

    // Test 6: %p (pointer)
    ptr = (void *)0x12345678;
    serial_printf("test 6 - [0x12345678]:[%p]\n", ptr);

    // Test 7: %d (negative)
    num1 = -500;
    serial_printf("test 7 - [-500]:[%d]\n", num1);

    // Test 8: %x with # flag
    num1 = 255;
    serial_printf("test 8 - [0xff]:[%#x]\n", num1);

    // Test 9: %08x (zero-padded)
    num1 = 0x7b;
    serial_printf("test 9 - [0000007b]:[%08x]\n", num1);

    // Test 10: %8d (space-padded)
    num1 = 123;
    serial_printf("test 10 - [     123]:[%8d]\n", num1);

    // Test 11: %s with null string
    str = NULL;
    serial_printf("test 11 - [(null)]:[%s]\n", str);

    // Test 12: %% (literal %)
    serial_printf("test 12 - [%]:[%%]\n");

    // Test 13: %d with zero
    num1 = 0;
    serial_printf("test 13 - [0]:[%d]\n", num1);

    // Test 14: %x with zero
    num1 = 0;
    serial_printf("test 14 - [0]:[%x]\n", num1);

    // Test 15: %u with zero
    unum1 = 0;
    serial_printf("test 15 - [0]:[%u]\n", unum1);

    // Test 16: %c with null char
    ch = '\0';
    serial_printf("test 16 - []:[%c]\n", ch);

    // Test 17: %s with empty string
    str = "";
    serial_printf("test 17 - []:[%s]\n", str);

    // Test 18: %p with null pointer
    ptr = NULL;
    serial_printf("test 18 - [0x00000000]:[%p]\n", ptr);

    // Test 19: %08x with # flag
    num1 = 0xabc;
    serial_printf("test 19 - [0x0000abc]:[%#08x]\n", num1);

    // Test 20: %10s (space-padded)
    str = "test";
    serial_printf("test 20 - [      test]:[%10s]\n", str);

    // Test 21: %.5s (precision)
    str = "longstring";
    serial_printf("test 21 - [longs]:[%.5s]\n", str);

    // Test 22: %10.5s (width and precision)
    str = "hello";
    serial_printf("test 22 - [     hello]:[%10.5s]\n", str);

    // Test 23: %d with large number
    num1 = 2147483647;
    serial_printf("test 23 - [2147483647]:[%d]\n", num1);

    // Test 24: %d with min int
    num1 = -2147483648;
    serial_printf("test 24 - [-2147483648]:[%d]\n", num1);

    // Test 25: %u with max unsigned
    unum1 = 4294967295U;
    serial_printf("test 25 - [4294967295]:[%u]\n", unum1);

    // Test 26: %x with large hex
    num1 = 0xffffffff;
    serial_printf("test 26 - [ffffffff]:[%x]\n", num1);

    // Test 27: %x with # flag
    num1 = 0xffffffff;
    serial_printf("test 27 - [0xffffffff]:[%#x]\n", num1);

    // Test 28: %p with large address
    ptr = (void *)0xdeadbeef;
    serial_printf("test 28 - [0xdeadbeef]:[%p]\n", ptr);

    // Test 29: %8d with negative
    num1 = -123;
    serial_printf("test 29 - [    -123]:[%8d]\n", num1);

    // Test 30: %08d with negative
    num1 = -123;
    serial_printf("test 30 - [-0000123]:[%08d]\n", num1);

    // Test 31: %x with small value
    num1 = 1;
    serial_printf("test 31 - [1]:[%x]\n", num1);

    // Test 32: %c with special char
    ch = '\n';
    serial_printf("test 32 - [\n]:[%c]\n", ch);

    // Test 33: %s with spaces
    str = "hello world";
    serial_printf("test 33 - [hello world]:[%s]\n", str);

    // Test 34: %12s with short string
    str = "hi";
    serial_printf("test 34 - [          hi]:[%12s]\n", str);

    // Test 35: %.2s with long string
    str = "abcdef";
    serial_printf("test 35 - [ab]:[%.2s]\n", str);

    // Test 36: %x with # and width
    num1 = 0x123;
    serial_printf("test 36 - [   0x123]:[%#8x]\n", num1);

    // Test 37: %d with multiple digits
    num1 = 987654;
    serial_printf("test 37 - [987654]:[%d]\n", num1);

    // Test 38: %u with small value
    unum1 = 5;
    serial_printf("test 38 - [5]:[%u]\n", unum1);

    // Test 39: %p with zero-padded width
    ptr = (void *)0xabc;
    serial_printf("test 39 - [  0x00000abc]:[%12p]\n", ptr);

    // Test 40: %s with special characters
    str = "test\n\t";
    serial_printf("test 40 - [test\n\t]:[%s]\n", str);

    // Test 41: %d with width and zero
    num1 = 0;
    serial_printf("test 41 - [00000000]:[%08d]\n", num1);

    // Test 42: Invalid specifier
    num1 = 42;
    serial_printf("test 42 - [%z42]:[%z42]\n", num1);

    // Test 43: Multiple specifiers
    num1 = 100;
    str = "multi";
    ch = 'X';
    serial_printf("test 43 - [100 multi X]:[%d %s %c]\n", num1, str, ch);

    // Test 44: %u with width
    unum1 = 456;
    serial_printf("test 44 - [   456]:[%6u]\n", unum1);

    // Test 45: %x with zero and # flag
    num1 = 0;
    serial_printf("test 45 - [0x0]:[%#x]\n", num1);

    // Test 46: %s with precision and null
    str = NULL;
    serial_printf("test 46 - [(nu]:[%.3s]\n", str);

    // Test 47: %p with small address
    ptr = (void *)0x1;
    serial_printf("test 47 - [0x00000001]:[%p]\n", ptr);

    // Test 48: %d with large width
    num1 = 99;
    serial_printf("test 48 - [          99]:[%12d]\n", num1);

    // Test 49: %x with uppercase hex (lowercase expected)
    num1 = 0xABC;
    serial_printf("test 49 - [abc]:[%x]\n", num1);

    // Test 50: Complex combination
    num1 = -42;
    unum1 = 0x1ff;
    str = "end";
    ptr = (void *)0x1234;
    serial_printf("test 50 - [-42 0x1ff end 0x00001234]:[%d %#x %s %p]\n", num1, unum1, str, ptr);
}

// Kernel initialization
static void kernel_init(multiboot_info_t* mbi) {
    _mbi = mbi;
    _hide_cursor();
    // stdio_init();
    // init_gdt();
    init_paging_stage2(_mbi);
    // Initialize IDT (sets up exceptions and IRQs)
    enable_fpu();
    load_multiboot_mods(_mbi);
    init_drivers();
    // init_processes();

    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    serial_puts("Hello, Serial Port!\n");
    // test_ide_write();
    test_mbr_parse();
    // serial_printf_test();
    // serial_printf("Debug: int=%d, str=%s, hex=%x, char=%c\n", 42, "test", 0xdeadbeef, 'A');
    // struct block_dev *dev = ide_get_block_dev();
    // serial_printf(" 1 ");
    // uint8_t buffer[512];
    // dev->read(0, buffer, 1);
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // printf("MBR: %02x %02x\n", buffer[510], buffer[511]);    
    // serial_printf("MBR: %x %x\n", buffer[510], buffer[511]);
    while(1);
    // print_module_info(mbi);
    // draw_background(mbi);
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    uint32_t tmpcolor;
    // vbe_set_fg_color(0xFF,0xFF ,0xFF );
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