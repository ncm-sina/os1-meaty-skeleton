#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #include <kernel/mconio.h>
// #include <kernel/mport.h>
#include <kernel/drivers/vga.h>
#include <kernel/arch/i386/mcpuid.h>
#include <kernel/arch/i386/paging.h>
#include <kernel/arch/i386/idt.h>
#include <kernel/arch/i386/isrs/all.h>
#include <kernel/drivers/all.h>

#include <kernel/kernel-base.h>
#include <kernel/basic_tests.h>
#include <io.h>

#include <stdio.h>

// Basic variables for testing
const char *_b_test_name = "Dave";
int _b_test_num = 42;
char _b_test_c = '!';
int _b_test_negative = -123;
unsigned int _b_test_hex_val = 28; // 0x1c


void b_test_mouse(void){
    // Mouse Tests
    printf("\n\n\nMouse Tests:\n");
    // printf("Initial X: %d, Y: %d, Buttons: %02X\n", 
    //         mouse_drv.get_x(), mouse_drv.get_y(), mouse_drv.get_buttons());

    // // Test 1: Set speed and move mouse
    // mouse_drv.set_speed(20); // Double normal speed
    // printf("Set speed to 20 (2x normal)\n");

    // // Test 2: Set resolution
    // mouse_drv.set_resolution(4); // 4 counts/mm
    // printf("Set resolution to 2 (4 counts/mm)\n");

    // // Test 3: Set sample rate
    // mouse_drv.set_sample_rate(100); // 100 packets/sec
    // printf("Set sample rate to 100 packets/sec\n");

    // // Test 4: Disable and re-enable mouse
    // mouse_drv.disable_mouse();
    // printf("Mouse disabled (move it, no updates)\n");
    // for (volatile int i = 0; i < 10000000; i++); // Delay to observe
    // mouse_drv.enable_mouse();
    // printf("Mouse re-enabled\n");

    uint32_t last_tick = timer_drv.get_ticks();
    int32_t last_x = mouse_drv.get_x();
    int32_t last_y = mouse_drv.get_y();
    uint8_t last_buttons = mouse_drv.get_buttons();
    int counter=0;
    while (1) {
        counter ++;

        uint32_t current_tick = timer_drv.get_ticks();
        struct key_event event;

        int32_t x = mouse_drv.get_x();
        int32_t y = mouse_drv.get_y();
        uint8_t buttons = mouse_drv.get_buttons();
        int32_t dx = mouse_drv.get_dx();
        int32_t dy = mouse_drv.get_dy();
        if (x != last_x || y != last_y || buttons != last_buttons || 1) {
            last_x = x;
            last_y = y;
            last_buttons = buttons;
            vga_gotoxy(0, 0);
            printf("c:%08X Mouse X: %04d  Y: %04d  Buttons: %02X  DX: %04d  DY: %04d  ", counter, 
                    x, y, buttons, dx, dy);
        }
    }    
}

// Test floating-point formatting
void b_test_floating_point(void) {
    float pi_float = 3.14159f;
    double pi_double = 3.14159265359;
    float neg_float = -42.567f;
    double small_double = 0.000123456789;

    // Default %f (6 digits)
    printf("Pi (float): %f", pi_float);         // "3.141590"
    printf("Pi (double): %f", pi_double);       // "3.141593"

    // Width and precision
    printf("Pi (6.2f): %6.2f", pi_double);      // "  3.14"
    printf("Neg (8.3f): %8.3f", neg_float);     // " -42.567"
    printf("Small (10.8f): %10.8f", small_double); // " 0.00012346"

    // Zero precision
    printf("Pi (0f): %.0f", pi_double);         // "3"

    // Sprintf test
    char test_buffer[64];
    sprintf(test_buffer, "Float test: %7.4f", pi_float);
    printf("Sprintf float: %s", test_buffer);    // "Float test:  3.1416"
}

void b_test_isr_driver(){
    printf("\n\n\n\n");
    printf("Kernel Loaded\n");
    printf("IDT Offset: %08X\n", idt_desc.offset);
    printf("IDT Size: %08X\n", idt_desc.size);
    uint32_t eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    printf("EFLAGS: %08X\n", eflags);
    printf("Initial Ticks: %08X\n", timer_drv.get_ticks());

    uint32_t last_tick = timer_drv.get_ticks();
    while (1) {
        uint32_t current_tick = timer_drv.get_ticks();
        struct key_event event;
        if (keyboard_drv.get_event(&event)) {
            char ascii = keyboard_drv.keycode_to_ascii(event.code, event.modifiers);
            vga_gotoxy(0, 1);
            printf("Key: %04X  Type: %d  Mod: %02X  ASCII: %c  ",
                    event.code, event.type, event.modifiers, ascii ? ascii : ' ');
            // Clear buffer on Enter press (for demo)
            if (event.code == KEY_ENTER && event.type == KEY_PRESS) {
                keyboard_drv.clear_buffer();
                printf("\nBuffer cleared\n");
            }
        }
        if (current_tick != last_tick) {
            last_tick = current_tick;
            vga_gotoxy(0, 0);
            printf("Ticks: %08X", current_tick);
        }
    }    
}

void b_test_idt(){
    // needs work not a proper test
    // printf("\n\n"); // Couple of enters at the start
    // printf("Kernel Loaded\n");
    // printf("IDT Offset: %08X\n", idt_desc.offset);
    // printf("IDT Size: %08X\n", idt_desc.size);
    // uint32_t eflags;
    // asm volatile("pushf; pop %0" : "=r"(eflags));
    // printf("EFLAGS: %08X\n", eflags);
    // Inspect IDT entry 32
    // uint32_t isr32_addr = (idt[32].offset_high << 16) | idt[32].offset_low;
    // printf("IDT[32] Address: %08X\n", isr32_addr);
    // printf("IDT[32] Selector: %04X\n", idt[32].selector);
    // printf("IDT[32] Type_Attr: %02X\n", idt[32].type_attr);    

}

void b_test_mbi(multiboot_info_t* mbi){
    printf("flags=%08X, mem_lower=%08X, mem_upper=%08X, boot_device=%08X \n", mbi->flags, mbi->mem_lower, mbi->mem_upper, mbi->boot_device);
    printf("cmdline=%08X, mods_count=%08X, mods_addr=%08X, syms=%08X \n", mbi->cmdline, mbi->mods_count, mbi->mods_addr, mbi->syms);
    printf("mmap_length=%08X, mmap_addr=%08X \n", mbi->mmap_length, mbi->mmap_addr);
    printf("\n");
}

void b_test_a20(){
	int r = is_A20_on();
	printf("a20 status[%d]: \n", r);
}

void b_test_multiboot_header(){
	// printf("multiboot_header[%s]: %#08x\n", "magic", multiboot_header.magic);
	// printf("multiboot_header[%s]: %#08x\n", "flags", multiboot_header.flags);
	// printf("multiboot_header[%s]: %#08x\n", "checksum", multiboot_header.checksum);
	// printf("multiboot_header[%s]: %#08x\n", "header_addr", multiboot_header.header_addr);
	// printf("multiboot_header[%s]: %#08x\n", "load_addr", multiboot_header.load_addr);
	// printf("multiboot_header[%s]: %#08x\n", "load_end_addr", multiboot_header.load_end_addr);
	// printf("multiboot_header[%s]: %#08x\n", "bss_end_addr", multiboot_header.bss_end_addr);
	// printf("multiboot_header[%s]: %#08x\n", "entry_addr", multiboot_header.entry_addr);
	// printf("multiboot_header[%s]: %#08x\n", "mode_type", multiboot_header.mode_type);
	// printf("multiboot_header[%s]: %#08x\n", "width", multiboot_header.width);
	// printf("multiboot_header[%s]: %#08x\n", "height", multiboot_header.height);
	// printf("multiboot_header[%s]: %#08x\n", "depth", multiboot_header.depth);
}

void b_test_mconio(){
    // Example 1: Simple printf with ANSI colors
    printf("Hello, \033[32;40m%s\033[0m! Num: %d%%\n", _b_test_name, _b_test_num);
    
    // Example 2: printf with tab and percentage
    printf("Tab test:\tThis is tabbed, %s. Percent: %d%%\n", _b_test_name, _b_test_num);
 
    // Example 3: Hexadecimal lowercase with leading zeros
    printf("Hex lowercase: %08x\n", _b_test_hex_val); // "0000001c"
    printf("Hex short: %04x\n", _b_test_hex_val);    // "001c"

    // Example 4: Hexadecimal uppercase with prefix
    printf("Hex uppercase with prefix: %#08X\n", _b_test_hex_val); // "0X0000001C"
    printf("Hex short with prefix: %#X\n", _b_test_hex_val);       // "0X1C"

    // Example 5: Mixed formats with hex and colors
    printf("Mixed: \033[31m%s\033[0m, Dec: %d, Hex: %#04x\n", _b_test_name, _b_test_num, _b_test_hex_val);

    // Example 6: Negative number and gotoxy
    vga_gotoxy(0, 6);
    printf("Negative number: %d\n", _b_test_negative);

    // Example 7: Using sprintf to build a string with hex
    char test_buffer[64];
    sprintf(test_buffer, "Hex test: %#08x, %s scored %d", _b_test_hex_val, _b_test_name, _b_test_num);
    vga_gotoxy(0, 7);
    printf("Sprintf result: %s\n", test_buffer);

    // Example 8: Changing text color and hex
    vga_set_textcolor(VGA_COLOR_CYAN, VGA_COLOR_DARK_GREY);
    printf("Cyan hex: %#x%%\n", _b_test_hex_val);
    vga_reset_textcolor();

    // Example 9: Tab size adjustment with hex
    vga_set_tabsize(4);
    printf("Smaller tab:\t%x\n", _b_test_hex_val);
    vga_set_tabsize(8); // Reset to default

}

void b_test_mconio_scroll(){
    // Example 10: Scrolling with hex values
    for (int i = 0; i < 30; i++) {
        printf("Line %d, Hex: %08x\n", i, i * _b_test_hex_val);
    }

}

void b_test_cupid(){
    char vendor[13];
    uint32_t family, model, stepping;

    // Check CPUID support
    if (!is_cpuid_supported()) {
        printf("CPUID not supported!\n");
    }

    // Get and print vendor string
    get_cpu_vendor(vendor);
    printf("CPU Vendor: %s\n", vendor);

    // Get and print CPU info
    get_cpu_info(&family, &model, &stepping);
    printf("Family: %u, Model: %u, Stepping: %u\n", family, model, stepping);

    // Check for specific features
    printf("SSE2 supported: %s\n", has_cpu_feature(FEATURE_SSE2) ? "Yes" : "No");
    printf("AVX supported: %s\n", has_cpu_feature(FEATURE_AVX) ? "Yes" : "No");

}