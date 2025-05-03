/* Multiboot constants */
.set ALIGN,    1<<0             /* Align modules on page boundaries */
.set MEMINFO,  1<<1             /* Request memory map from GRUB */
.set MULTIBOOT_VIDEO_MODE, (1<<2)/*| 1<<11 | 1<<12*/
.set FLAGS,    ALIGN | MEMINFO  /* Combined flags */
.set MAGIC,    0x1BADB002       /* Multiboot magic number */
.set CHECKSUM, -(MAGIC + FLAGS) /* Checksum for validation */

# Kernel virtual base address
.set KERNEL_VIRT_BASE, 0xC0000000
.set PAGETABLES_MAXSIZE, 1280
.set KERNEL_PAGEDIR,  (page_directory - KERNEL_VIRT_BASE)
.set MAX_PROCESSES, 256

/* Multiboot header for GRUB */
.section .multiboot
.global multiboot_header
.align 4
multiboot_header:
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
    /* Optional fields (unused here) */
    .long 0  /* header_addr */
    .long 0  /* load_addr */
    .long 0  /* load_end_addr */
    .long 0  /* bss_end_addr */
    .long 0  /* entry_addr */
    .long 0  /* mode_type */
    .long 0  /* width 1024*/
    .long 0  /* height 768*/
    .long 0  /* depth 32*/

/* Kernel entry point */
.section .text
.global _start
_start:
	movl %ebx, multiboot_info - KERNEL_VIRT_BASE

    # temporary stack
    mov 0x9000, %esp
    call init_paging_stage1 # - KERNEL_VIRT_BASE
    call init_gdt

    movl $stack_top, %esp
    pushl multiboot_info
    call kernel_main
    cli
1:  hlt
    jmp  1b
.size _start, . - _start


/* Initialized data */
.section .data
.align 16
/* GDT */
gdt_start:
    .long 0x00000000
    .long 0x00000000
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x9A
    .byte 0xCF
    .byte 0x00
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x92
    .byte 0xCF
    .byte 0x00
gdt_end:
.align 16
gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start 

.global multiboot_info
multiboot_info: .long 0
/* BSS */
.section .bss
.align 4096
.global kernel_pagedir
kernel_pagedir: .skip 4096
.global pagetables
pagetables: .skip 4096 * PAGETABLES_MAXSIZE
.global pagedirs
pagedirs: .skip 4096 * MAX_PROCESSES
stack_bottom: .skip 16384
stack_top: