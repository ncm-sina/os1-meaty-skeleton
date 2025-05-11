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
