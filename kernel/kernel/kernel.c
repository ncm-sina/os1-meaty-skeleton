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

#include <kernel/mconio.h>
#include <kernel/mport.h>
#include <kernel/vga_basic.h>

#include <kernel/kernel-base.h>
// typedef struct {
// 	uint32_t magic; //	required
// 	uint32_t flags; //	required
// 	uint32_t checksum; //	required
// 	uint32_t header_addr; //	if flags[16] is set
// 	uint32_t load_addr; //	if flags[16] is set
// 	uint32_t load_end_addr; //	if flags[16] is set
// 	uint32_t bss_end_addr; //	if flags[16] is set
// 	uint32_t entry_addr; //	if flags[16] is set
// 	uint32_t mode_type; //	if flags[2] is set
// 	uint32_t width; //	if flags[2] is set
// 	uint32_t height; //	if flags[2] is set
// 	uint32_t depth; //	if flags[2] is set
// } MULTIBOOT_HEADER;

// extern MULTIBOOT_HEADER multiboot_header;

// // Declare the assembly function as extern
// extern int is_A20_on(void);

// Static function to hide the cursor using port I/O
static void _hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
}

// Kernel initialization
static void kernel_init(void) {
    _hide_cursor();
}

// Kernel main function
void kernel_main(void) {
    kernel_init();
    // clrscr();

    // Basic variables for testing
    const char *name = "Dave";
    int num = 42;
    char c = '!';
    int negative = -123;
    unsigned int hex_val = 28; // 0x1c

	int r = is_A20_on();

	cprintf("a20 status[%d]: \n", r);

	// cprintf("multiboot_header[%s]: %#08x\n", "magic", multiboot_header.magic);
	// cprintf("multiboot_header[%s]: %#08x\n", "flags", multiboot_header.flags);
	// cprintf("multiboot_header[%s]: %#08x\n", "checksum", multiboot_header.checksum);
	// cprintf("multiboot_header[%s]: %#08x\n", "header_addr", multiboot_header.header_addr);
	// cprintf("multiboot_header[%s]: %#08x\n", "load_addr", multiboot_header.load_addr);
	// cprintf("multiboot_header[%s]: %#08x\n", "load_end_addr", multiboot_header.load_end_addr);
	// cprintf("multiboot_header[%s]: %#08x\n", "bss_end_addr", multiboot_header.bss_end_addr);
	// cprintf("multiboot_header[%s]: %#08x\n", "entry_addr", multiboot_header.entry_addr);
	// cprintf("multiboot_header[%s]: %#08x\n", "mode_type", multiboot_header.mode_type);
	// cprintf("multiboot_header[%s]: %#08x\n", "width", multiboot_header.width);
	// cprintf("multiboot_header[%s]: %#08x\n", "height", multiboot_header.height);
	// cprintf("multiboot_header[%s]: %#08x\n", "depth", multiboot_header.depth);



    // Example 1: Simple cprintf with ANSI colors
    cprintf("Hello, \033[32;40m%s\033[0m! Num: %d%%\n", name, num);
    
    // Example 2: printf with tab and percentage
    printf("Tab test:\tThis is tabbed, %s. Percent: %d%%\n", name, num);

    // Example 3: Hexadecimal lowercase with leading zeros
    cprintf("Hex lowercase: %08x\n", hex_val); // "0000001c"
    cprintf("Hex short: %04x\n", hex_val);    // "001c"

    // Example 4: Hexadecimal uppercase with prefix
    cprintf("Hex uppercase with prefix: %#08X\n", hex_val); // "0X0000001C"
    cprintf("Hex short with prefix: %#X\n", hex_val);       // "0X1C"

    // Example 5: Mixed formats with hex and colors
    cprintf("Mixed: \033[31m%s\033[0m, Dec: %d, Hex: %#04x\n", name, num, hex_val);

    // Example 6: Negative number and gotoxy
    gotoxy(0, 6);
    printf("Negative number: %d\n", negative);

    // Example 7: Using sprintf to build a string with hex
    char test_buffer[64];
    sprintf(test_buffer, "Hex test: %#08x, %s scored %d", hex_val, name, num);
    gotoxy(0, 7);
    printf("Sprintf result: %s\n", test_buffer);

    // Example 8: Changing text color and hex
    set_textcolor(VGA_COLOR_CYAN, VGA_COLOR_DARK_GREY);
    printf("Cyan hex: %#x%%\n", hex_val);
    _vgab_reset_textcolor();

    // Example 9: Tab size adjustment with hex
    _vgab_set_tabsize(4);
    printf("Smaller tab:\t%x\n", hex_val);
    _vgab_set_tabsize(8); // Reset to default

    // // Example 10: Scrolling with hex values
    // for (int i = 0; i < 30; i++) {
    //     printf("Line %d, Hex: %08x\n", i, i * hex_val);
    // }
}