/* Multiboot constants */
.set ALIGN,    1<<0             /* Align modules on page boundaries */
.set MEMINFO,  1<<1             /* Request memory map from GRUB */
.set MULTIBOOT_VIDEO_MODE, (1<<2)/*| 1<<11 | 1<<12*/
.set FLAGS,    ALIGN | MEMINFO  /* Combined flags */
.set MAGIC,    0x1BADB002       /* Multiboot magic number */
.set CHECKSUM, -(MAGIC + FLAGS) /* Checksum for validation */


/* Multiboot header for GRUB */
.section .multiboot
.global multiboot_header
.align 16
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

# Kernel virtual base address
.set KERNEL_VIRT_BASE, 0xC0000000
.set PAGE_SIZE, 4096
.set PAGE_TABLE_SIZE, 1024
.set PAGETABLES_MAXSIZE, 1280
.set KERNEL_PAGEDIR,  (page_directory - KERNEL_VIRT_BASE)
.set MAX_PROCESSES, 256
.set GDT_PHYS_ADDR, 0x8000
.set GDT_DESCRIPTOR_PHYS_ADDR, 0x80F0

.set KCODE32_SEG, 0x08     # 32-bit code segment
.set KDATA32_SEG, 0x10     # 32-bit data segment
.set UCODE32_SEG, 0x18     # 16-bit code segment
.set UDATA32_SEG, 0x20     # 16-bit data segment
.set CODE16_SEG, 0x28     # 16-bit code segment
.set DATA16_SEG, 0x30     # 16-bit data segment


/* Kernel entry point */
.section .text
.global _start
_start:
    # movl %eax, %ecx         /* Save magic number (eax) */
	movl %ebx, multiboot_info

    call init_gdt

    ljmp $KCODE32_SEG, $2f
2:

    movl $stack_top, %esp
    pushl multiboot_info
    pushl $0x1BADB002
    call installer_main
    cli
1:  hlt
    jmp  1b
.size _start, . - _start


/* Initialized data */
.section .data
.align 16
.global multiboot_info
multiboot_info: .long 0
/* BSS */
.section .bss
.align 16
.global last_pagetable_idx
last_pagetable_idx: .long 0
.align 4096
.global kernel_pagedir
kernel_pagedir: .skip 4096
.global pagetables
pagetables: .skip 4096 * PAGETABLES_MAXSIZE
.global pagedirs
pagedirs: .skip 4096 * MAX_PROCESSES
stack_bottom: .skip 16384
stack_top: