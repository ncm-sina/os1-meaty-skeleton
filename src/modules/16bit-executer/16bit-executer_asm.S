# mode_switch.S
# Assembly code for mode switching

.set SWITCH_TEXT_PHYS_ADDR, 0x1020
.set SWITCH_DATA_PHYS_ADDR, 0x1F00
# .set SWITCH_STACK_PHYS_ADDR, 0x1FFF
.set RM_PROGRAM_TEXT_PHYS_ADDR, 0x03000
.set RM_PROGRAM_STACK_PHYS_ADDR, 0x06FFF
.set RM_PROGRAM_DATA_PHYS_ADDR, 0x06000

.set KERNEL_VIRT_BASE, 0xC0000000


.section .text
.global _mstart
_mstart:
.global _exec_rm_program
_text_start:
    jmp _exec_rm_program

# Export run_rm_program address
.global run_rm_program_ptr
run_rm_program_ptr: .long _exec_rm_program

.code32
_exec_rm_program:
    pushl %ebp
    movl %esp, %ebp
    pushl %eax

    # typedef struct {
    #  8(%ebp)   uint32_t entry_addr;
    #  12(%ebp)   uint32_t text_start_addr; // virtual address of binary start 
    #  16(%ebp)   uint16_t text_size; // virtual address of binary start
    #  18(%ebp)   uint32_t data_start_addr; // virtual address of binary start
    #  22(%ebp)   uint16_t data_size; // virtual address of binary start
    # } rm_binary_info_t;

    movl 8(%ebp), %eax
    movl %eax, _main_func_ptr


    # call _load_rm_program
    # Copy rm program text to 'RM_PROGRAM_TEXT_PHYS_ADDR'
    mov 12(%ebp), %esi
    movl 16(%ebp), %ecx
    mov $RM_PROGRAM_TEXT_PHYS_ADDR, %edi
    cld
    rep movsb

    # Copy rm program data to 'RM_PROGRAM_DATA_PHYS_ADDR' 
    mov 20(%ebp), %esi
    movl 24(%ebp), %ecx
    mov $RM_PROGRAM_DATA_PHYS_ADDR, %edi
    cld
    rep movsb
#   end of _load_rm_program


#    call _run_rm_program
    # Save protected-mode state (e.g., GDTR, IDTR, %cr3)
    sgdt saved_gdt
    sidt saved_idt
    mov %cr3, %eax
    mov %eax, saved_cr3

    pushf
    pop %eax
    test $0x200, %eax   # Check IF flag (bit 9)
    jz .interrupts_off
    movl $1, is_int_enabled      # Store 1 if interrupts enabled
    jmp .continue
.interrupts_off:
    movl $0, is_int_enabled      # Store 0 if interrupts disabled
.continue:

    # we push all except eax because it will be used to hold return value

    movl %esp, saved_esp
    movl %ebp, saved_ebp

    cli

    lgdt tmpgdt_32_descriptor
    ljmp $0x08, $1f
1:

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # call *enter_real_mode_ptr
    # Assume we're in a 1:1 mapped page
    # Disable paging
    mov %cr0, %eax
    and $0x7FFFFFFF, %eax    # Clear PG bit
    mov %eax, %cr0

    # Flush TLB
    xor %eax, %eax
    mov %eax, %cr3


    # Disable protected mode
    mov %cr0, %eax
    and $0xFFFFFFFE, %eax    # Clear PE bit
    mov %eax, %cr0

.code16
    ljmp $0, $2f
2:
#    lgdt gdt_16_descriptor
# ljmp $0x0000, $3f
# 3:

# 16-bit code

    # Load real mode data segments
    mov %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    mov $RM_PROGRAM_STACK_PHYS_ADDR, %ax
    mov %ax, %bp
    mov %bp, %sp

    # Load real mode IDT
    lidt real_mode_idt
    # Enable interrupts
    sti

#    call 0x3000

    mov $0x0E41, %ax        # AH=0x0E (teletype), AL='A'
    mov $0x0017, %bx        # Page 0, no color
    int $0x10

    mov %ax, program_return_value

    # Enable protected mode
    cli

    lgdt tmpgdt_32_descriptor

    mov %cr0, %eax
    or $0x1, %eax           # Set PE bit
    mov %eax, %cr0

    # Far jump to 32-bit protected mode
    ljmp $0x08, $4f
4:

.code32
return_to_protected_mode:
    # Reload segment registers
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    movl saved_esp, %esp
    movl saved_ebp, %ebp

    # Re-enable paging
    mov saved_cr3, %eax     # Assuming page directory at 1MB
    mov %eax, %cr3
    mov %cr0, %eax
    or $0x80000000, %eax    # Set PG bit
    mov %eax, %cr0

    ljmp $0x08, $5f # + KERNEL_VIRT_BASE
5:
# call return_to_protected_mode

    # Reload original GDT (assuming it's at 0x1000)

    lgdt saved_gdt
    ljmp $0x08, $6f # + KERNEL_VIRT_BASE
6:

    lidt saved_idt
    ljmp $0x08, $7f
7:

    # Restore interrupt state
    mov $1, %eax
    cmp $is_int_enabled, %eax
    jne .no_sti
    sti                 # Re-enable interrupts if previously enabled
    jmp .continue2
.no_sti:
    cli
.continue2:

# end of return_to_protected_mode

    movzwl program_return_value, %eax

    popl %eax
#    movl %ebp, %esp
    popl %ebp
    ret


_text_end:

# .align 0x1000
.section .data
_data_start:
gdt_16: 
    # Null descriptor
    .word 0x0000
    .word 0x0000
    .word 0x0000
    .word 0x0000

    # 16-bit code segment
    .word 0xFFFF        # Limit 0-15
    .word 0x0000        # Base 0-15
    .byte 0x00          # Base 16-23
    .byte 0x9A          # Access byte: present, ring 0, code, executable, readable
    .byte 0x00          # Granularity: limit 16-19 = 0, 16-bit
    .byte 0x00          # Base 24-31

    # 16-bit data segment
    .word 0xFFFF        # Limit 0-15
    .word 0x0000        # Base 0-15
    .byte 0x00          # Base 16-23
    .byte 0x92          # Access byte: present, ring 0, data, writable
    .byte 0x00          # Granularity: limit 16-19 = 0, 16-bit
    .byte 0x00          # Base 24-31

tmpgdt_32: 
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

gdt_16_descriptor: 
    .word 0x17          # Size of GDT - 1
    .long gdt_16        # Address of GDT

tmpgdt_32_descriptor: 
    .word 0x17          # Size of GDT - 1
    .long tmpgdt_32        # Address of GDT


saved_esp: .long 0 
saved_ebp: .long 0 
saved_sp: .word 0 
saved_bp: .word 0 
saved_gdt:
.space 6
# .long 0x11112222
# .word 0x3333 
saved_idt:
.space 6
# .long 0x44445555
# .word 0x6666 
saved_cr3: .long 0 
is_int_enabled: .word 0 
enter_real_mode_ptr: .long 0 
_main_func_ptr: .long 0 
# 16-bit GD

real_mode_idt: 
    .word 0x3FF         # Limit
    .long 0x0           # Base
program_return_value:
    .word 0
_data_end: