/* Multiboot constants */
.set ALIGN,    1<<0             /* Align modules on page boundaries */
.set MEMINFO,  1<<1             /* Request memory map from GRUB */
.set PAGING,  1<<1             /* Request memory map from GRUB */
.set FLAGS,    ALIGN | MEMINFO  /* Combined flags */
.set MAGIC,    0x1BADB002       /* Multiboot magic number */
.set CHECKSUM, -(MAGIC + FLAGS) /* Checksum for validation */

# Kernel virtual base address
.set KERNEL_VIRT_BASE, 0xC0000000
.set KERNEL_PAGE_DIR,  (page_directory - KERNEL_VIRT_BASE)

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
    .long 0  /* width */
    .long 0  /* height */
    .long 0  /* depth */

/* Kernel entry point */
.section .text
.global _start
_start:
	movl %ebx, multiboot_info - KERNEL_VIRT_BASE

    /* Set up page table: map physical 0x0 - 0x400000 */
    movl $pagetable - KERNEL_VIRT_BASE, %edi
    movl $0, %eax
    movl $1024, %ecx
1:  orl  $3, %eax
    movl %eax, (%edi)
    addl $4, %edi
    addl $4096, %eax
    loop 1b

    /* Set up page directory */
    movl $pagedir - KERNEL_VIRT_BASE, %edi
    movl $pagetable - KERNEL_VIRT_BASE, %eax
    orl  $3, %eax
    movl %eax, (%edi)           /* PDE 0: 0x0 - 0x400000 */
    movl %eax, 768*4(%edi)      /* PDE 768: 0xC0000000 - 0xC0400000 */

    /* Enable paging */
    movl $pagedir - KERNEL_VIRT_BASE, %eax
    movl %eax, %cr3
    movl %cr0, %eax
    orl  $0x80000000, %eax
    movl %eax, %cr0

    /* Far jump to higher half */
	mov $higher_half, %eax
    jmp *%eax

higher_half:

    /* Load GDT */
    lgdt gdt_descriptor
    ljmp $0x08, $gdt_loaded

gdt_loaded:

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp
    pushl multiboot_info
    call kernel_main
    call blue_death_screen
    cli
1:  hlt
    jmp  1b
.size _start, . - _start

/* Blue Death Screen: Sets blue background, prints message, restarts */
blue_death_screen:
    /* Clear screen with blue background (VGA text mode at 0xC00B8000) */
    /* movl $0xC00B8000, %edi        VGA buffer virtual address */
    movl $0xB8000, %edi       /* VGA buffer virtual address */
    movl $2000, %ecx             /* 80x25 = 2000 characters */
    movw $0x1720, %ax            /* Blue background (0x17), space char (0x20) */
    rep  stosw                   /* Fill screen (2 bytes per char: attr + char) */

    /* Print "KERNEL PANIC" at top-left */
    ; movl $0xC00B8000, %edi       /* Reset EDI to VGA start */
    movl $0xB8000, %edi       /* Reset EDI to VGA start */
    add $1610, %edi       /* Reset EDI to VGA start */
    movl $panic_msg, %esi        /* Source string address */
    movb $0x1F, %ah              /* Blue bg, white fg */
    movl $68, %ecx               /* Length of "KERNEL PANIC" */
print_loop:
    lodsb                        /* Load next char from panic_msg into AL */
    stosw                        /* Write char (AL) + attribute (AH) to VGA */
    loop print_loop              /* Repeat for ECX times */

    /* Rough 2-second delay (no timer, just CPU cycles) */
    movl $0x8000000, %ecx        /* Arbitrary large number for delay */
delay_loop:

    loop delay_loop              /* Decrement ECX, loop until 0 */

    /* Restart via keyboard controller */
    movb $0xFE, %al              /* Pulse reset line */
    outb %al, $0x64              /* Write to port 0x64 */
    jmp  .                       /* Shouldn’t reach here (infinite loop) */
    ret                          /* Fallback (unreachable) */


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
panic_msg: .ascii "KERNEL PANIC"

/* BSS */
.section .bss
.align 4096
.global pagedir
pagedir: .skip 4096
.global pagetable
pagetable: .skip 4096
stack_bottom: .skip 16384
stack_top: