ENTRY(_start)                /* Entry point symbol for the kernel */

SECTIONS {
    /* Place the kernel at 3GB (0xC0000000) in virtual memory */
    . = 0xC0100000;         /* Kernel base address (1MB above 3GB for bootloader compatibility) */
    _kernel_start_virt = .;  /* Physical address of kernel start */
    _kernel_start_phys = . - 0xC0000000;  /* Physical address of kernel start */

    /* Multiboot header and initial code */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) {
        *(.multiboot)       /* Multiboot header section (if using a multiboot bootloader) */
        *(.text)            /* All executable code */
    }

    /* Read-only data */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) {
        *(.rodata*)         /* Read-only data (constants, strings, etc.) */
    }

    /* Read-write data */
    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) {
        *(.data)            /* Initialized data */
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) {
        *(COMMON)           /* Uninitialized global variables */
        *(.bss)             /* Zero-initialized data */
    }

    /* Define some symbols for the kernel */
    _kernel_end_virt = .;               /* Virtual address of kernel end */
    _kernel_end_phys = . - 0xC0000000;  /* Physical address of kernel end */
}