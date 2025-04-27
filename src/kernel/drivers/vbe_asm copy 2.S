# vbe_bios.S
# GAS (AT&T) syntax for i686-elf
# VESA VBE BIOS calls, executes real-mode stub at 0x1000

.section .text
.global vbe_bios_call
.global real_mode_code_start
.global real_mode_code_end
.set KERNEL_VIRT_BASE, 0xC0000000

# 32-bit code
.code32
vbe_bios_call:
    push %ebp
    mov %esp, %ebp
    push %ebx
    push %esi
    push %edi

    # Save EFLAGS to check interrupt state
    pushf
    pop %eax
    test $0x200, %eax   # Check IF flag (bit 9)
    jz .interrupts_off
    movl $1, 0x2020      # Store 1 if interrupts enabled
    jmp .continue
.interrupts_off:
    movl $0, 0x2020      # Store 0 if interrupts disabled
.continue:

    # Parameters: ax, bx, cx, dx, es_di (on stack at ebp+8, +12, +16, +20, +24)
    mov 8(%ebp), %eax   # AX
    mov 12(%ebp), %ebx  # BX
    mov 16(%ebp), %ecx  # CX
    mov 20(%ebp), %edx  # DX
    mov 24(%ebp), %edi  # ES:DI

    # Store parameters for real-mode code
    mov %ax, 0x2008     # AX
    mov %bx, 0x200C     # BX
    mov %cx, 0x2010     # CX
    mov %dx, 0x2014     # DX
    mov %di, 0x2018     # DI
    mov %edi, %eax
    shr $4, %eax
    mov %ax, 0x201A     # ES = es_di >> 4

    # Copy real-mode code to 0x1000
    mov $real_mode_code_start, %esi
    mov $0x1000, %edi
    mov $real_mode_code_end, %ecx
    sub %esi, %ecx      # ecx = size of real-mode code
    cld
    rep movsb

    # Set up real-mode segment registers
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Save protected mode state
    push %ds
    push %es
    push %fs
    push %gs
    cli
    mov %esp, 0x2000    # Save stack pointer

    # Load real-mode IDT
    # lidt real_mode_idt

    # we can have lidt 0x2028
    mov $0x3FF, %ax
    mov %ax, 0x2028  
    mov $0, %eax
    mov %eax, 0x202a  

    jmp .next - KERNEL_VIRT_BASE
.next:

    # Disable paging and protected mode
    mov %cr0, %eax
    mov %eax, 0x2030 # save cr0
    and $~0x80000001, %eax  # Clear PG and PE
    mov %eax, %cr0

# Switch to 16-bit code

    ljmp $0x0000, $real_mode_entry - KERNEL_VIRT_BASE

.code16
real_mode_entry:
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $0x7C00, %sp    # Real-mode stack

    lidt 0x2028
    sti

    # Call real-mode stub at 0x1000
    lcall $0x0000, $0x1000

# Switch to 32-bit code
.code32
    # Save return value (AX already at 0x2004 by stub)
 
    # Re-enable protected mode and paging (restoring cr0)
    mov 0x2030, %eax
    mov %eax, %cr0

    ljmp $0x08, $protected_mode_entry

protected_mode_entry:
    # Restore segment registers
    mov $0x10, %ax      # Data segment selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov 0x2000, %esp    # Restore stack

    # Restore IDT
    lidt protected_mode_idt

    # Restore interrupt state
    cmp $1, 0x2020
    jne .no_sti
    sti                 # Re-enable interrupts if previously enabled
    jmp .continue2
.no_sti:
    cli
.continue2:

    # Return AX
    mov 0x2004, %eax

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    ret

.section .data
real_mode_idt:
    .word 0x3FF         # IDT limit
    .long 0             # IDT base

protected_mode_idt:
    .word 0             # Set by OS
    .long 0             # Set by OS

.section .text
real_mode_code_start:
# 16-bit code
.code16
    mov 0x2008, %ax     # AX
    mov 0x200C, %bx     # BX
    mov 0x2010, %cx     # CX
    mov 0x2014, %dx     # DX
    mov 0x2018, %di     # DI
    mov 0x201A, %es     # ES
    int $0x10
    mov %ax, 0x2004     # Save AX
    lret                # Far return
real_mode_code_end: