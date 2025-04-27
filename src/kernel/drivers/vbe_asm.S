# vbe_bios.S
# GAS (AT&T) syntax for i686-elf
# VESA VBE BIOS calls, executes real-mode stub at 0x1000
.set RM_PROGRAM_TEXT_PHYS_ADDR, 0x02000
.set RM_PROGRAM_STACK_PHYS_ADDR, 0x07BFF
.set RM_PROGRAM_DATA_PHYS_ADDR, 0x06000

# .global _vbe_bc_ax
# .global _vbe_bc_bx
# .global _vbe_bc_cx
# .global _vbe_bc_dx
# .global _vbe_bc_di
# .global _vbe_bc_es

.global _vbe_bc_text_start
.global _vbe_bc_text_end

.global _vbe_bc_data_start
.global _vbe_bc_data_end

.section .data
_vbe_bc_data_start:
    _vbe_bc_ax: .word 0
    _vbe_bc_bx: .word 0
    _vbe_bc_cx: .word 0
    _vbe_bc_dx: .word 0
    _vbe_bc_di: .word 0
    _vbe_bc_es: .word 0
_vbe_bc_data_end:

.section .text
_vbe_bc_text_start:
.code16
    mov $0x0, %ax     # DS = 0x6000 >> 4
    mov %ax, %ds
    mov 0x6000, %ax     # Absolute address of _vbe_bc_ax
    mov 0x6002, %bx     # _vbe_bc_bx
    mov 0x6004, %cx     # _vbe_bc_cx
    mov 0x6006, %dx     # _vbe_bc_dx
    mov 0x6008, %di     # _vbe_bc_di
    mov 0x600A, %es     # _vbe_bc_es
    int $0x10
    ret
_vbe_bc_text_end:
