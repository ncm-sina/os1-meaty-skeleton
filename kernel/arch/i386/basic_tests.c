#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/mconio.h>
#include <kernel/mport.h>
#include <kernel/vga_basic.h>
#include <kernel/mcpuid.h>

#include <kernel/kernel-base.h>
#include <kernel/basic_tests.h>

// Basic variables for testing
const char *_b_test_name = "Dave";
int _b_test_num = 42;
char _b_test_c = '!';
int _b_test_negative = -123;
unsigned int _b_test_hex_val = 28; // 0x1c


void b_test_a20(){
	int r = is_A20_on();
	cprintf("a20 status[%d]: \n", r);
}

void b_test_multiboot_header(){
	cprintf("multiboot_header[%s]: %#08x\n", "magic", multiboot_header.magic);
	cprintf("multiboot_header[%s]: %#08x\n", "flags", multiboot_header.flags);
	cprintf("multiboot_header[%s]: %#08x\n", "checksum", multiboot_header.checksum);
	cprintf("multiboot_header[%s]: %#08x\n", "header_addr", multiboot_header.header_addr);
	cprintf("multiboot_header[%s]: %#08x\n", "load_addr", multiboot_header.load_addr);
	cprintf("multiboot_header[%s]: %#08x\n", "load_end_addr", multiboot_header.load_end_addr);
	cprintf("multiboot_header[%s]: %#08x\n", "bss_end_addr", multiboot_header.bss_end_addr);
	cprintf("multiboot_header[%s]: %#08x\n", "entry_addr", multiboot_header.entry_addr);
	cprintf("multiboot_header[%s]: %#08x\n", "mode_type", multiboot_header.mode_type);
	cprintf("multiboot_header[%s]: %#08x\n", "width", multiboot_header.width);
	cprintf("multiboot_header[%s]: %#08x\n", "height", multiboot_header.height);
	cprintf("multiboot_header[%s]: %#08x\n", "depth", multiboot_header.depth);
}

void b_test_mconio(){
    // Example 1: Simple cprintf with ANSI colors
    cprintf("Hello, \033[32;40m%s\033[0m! Num: %d%%\n", _b_test_name, _b_test_num);
    
    // Example 2: printf with tab and percentage
    printf("Tab test:\tThis is tabbed, %s. Percent: %d%%\n", _b_test_name, _b_test_num);
 
    // Example 3: Hexadecimal lowercase with leading zeros
    cprintf("Hex lowercase: %08x\n", _b_test_hex_val); // "0000001c"
    cprintf("Hex short: %04x\n", _b_test_hex_val);    // "001c"

    // Example 4: Hexadecimal uppercase with prefix
    cprintf("Hex uppercase with prefix: %#08X\n", _b_test_hex_val); // "0X0000001C"
    cprintf("Hex short with prefix: %#X\n", _b_test_hex_val);       // "0X1C"

    // Example 5: Mixed formats with hex and colors
    cprintf("Mixed: \033[31m%s\033[0m, Dec: %d, Hex: %#04x\n", _b_test_name, _b_test_num, _b_test_hex_val);

    // Example 6: Negative number and gotoxy
    gotoxy(0, 6);
    printf("Negative number: %d\n", _b_test_negative);

    // Example 7: Using sprintf to build a string with hex
    char test_buffer[64];
    sprintf(test_buffer, "Hex test: %#08x, %s scored %d", _b_test_hex_val, _b_test_name, _b_test_num);
    gotoxy(0, 7);
    printf("Sprintf result: %s\n", test_buffer);

    // Example 8: Changing text color and hex
    set_textcolor(VGA_COLOR_CYAN, VGA_COLOR_DARK_GREY);
    printf("Cyan hex: %#x%%\n", _b_test_hex_val);
    _vgab_reset_textcolor();

    // Example 9: Tab size adjustment with hex
    _vgab_set_tabsize(4);
    printf("Smaller tab:\t%x\n", _b_test_hex_val);
    _vgab_set_tabsize(8); // Reset to default

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