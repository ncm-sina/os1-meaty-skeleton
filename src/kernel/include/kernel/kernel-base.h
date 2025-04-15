#ifndef _KERNEL_BASE_H
#define _KERNEL_BASE_H

typedef struct {
	uint32_t magic; //	required
	uint32_t flags; //	required
	uint32_t checksum; //	required
	uint32_t header_addr; //	if flags[16] is set
	uint32_t load_addr; //	if flags[16] is set
	uint32_t load_end_addr; //	if flags[16] is set
	uint32_t bss_end_addr; //	if flags[16] is set
	uint32_t entry_addr; //	if flags[16] is set
	uint32_t mode_type; //	if flags[2] is set
	uint32_t width; //	if flags[2] is set
	uint32_t height; //	if flags[2] is set
	uint32_t depth; //	if flags[2] is set
} MULTIBOOT_HEADER;

extern MULTIBOOT_HEADER multiboot_header;

// Declare the assembly function as extern
extern int is_A20_on(void);

extern uint32_t _kernel_start;
extern uint32_t _kernel_bss_end;
extern uint32_t _kernel_end;

static inline void halt(void) {
    asm volatile ("cli; hlt");  // Halt CPU safely
}

// Static function to hide the cursor using port I/O
static void _hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
}

#endif