ENTRY(_start)                /* Entry point symbol for the kernel */

SECTIONS {
    . = 0xC0100000;            /* Start at 1MB (physical and virtual) */
    _kernel_start_virt = .;  /* Virtual address of kernel start */
    _kernel_start_phys = . - 0xC0000000;  /* Physical address of kernel start */


    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) {
        *(.multiboot)          /* Multiboot header */
        *(.text)            /* All executable code */
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) {
        *(.rodata*)         /* Read-only data */
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) {
        *(.data)            /* Initialized data */
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) {
        *(COMMON)           /* Uninitialized global variables */
        *(.bss)             /* Zero-initialized data */
    }


    /* Define kernel end symbols */
    _kernel_end_virt = .;               /* Virtual address of kernel end */
    _kernel_end_phys = . - 0xC0000000;  /* Physical address of kernel end */

}