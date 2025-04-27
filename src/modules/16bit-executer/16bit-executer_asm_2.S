# mode_switch.S
# Assembly code for mode switching

.set SWITCH_TEXT_PHYS_ADDR, 0x1020
.set SWITCH_DATA_PHYS_ADDR, 0x1F00
# .set SWITCH_STACK_PHYS_ADDR, 0x1FFF
.set RM_PROGRAM_TEXT_PHYS_ADDR, 0x02000
.set RM_PROGRAM_STACK_PHYS_ADDR, 0x07BFF
.set RM_PROGRAM_DATA_PHYS_ADDR, 0x06000

.set tmp_addr16_ptr, 0x1016
.set tmp_addr32_ptr, 0x101A

# .set _data_base, 0x1160
# .set saved_esp, _data_base + 0
# .set saved_ebp, _data_base + 4
# .set saved_sp, _data_base + 8
# .set saved_bp, _data_base + 10
# .set saved_gdt, _data_base + 12
# .set saved_idt, _data_base + 18
# .set saved_cr3, _data_base + 24
# .set is_int_enabled, _data_base + 28
# .set enter_real_mode_ptr, _data_base + 30
# .set _main_func_ptr, _data_base + 34
# .set gdt_16, _data_base + 38
# .set gdt_16_descriptor, _data_base + 62
# .set real_mode_idt, _data_base + 68
# .set program_return_value, _data_base + 74

# .set _ljmp1, SWITCH_TEXT_PHYS_ADDR - *_text_start + $1f

.set KERNEL_VIRT_BASE, 0xC0000000

# .extern pagedir
# .extern run_rm_program

# .section .header
# .global _header
# _header:
#    .long _text_start2  # Offset of .text
#    .long _text_end2    # End of .text
#    .long _data_start2  # Offset of .data
#    .long _data_end2    # End of .data

.section .text
.global _mstart
_mstart:
.global _exec_rm_program
_text_start:
    jmp _exec_rm_program

# Export run_rm_program address
.global run_rm_program_ptr
run_rm_program_ptr: .long _exec_rm_program

# tmp_addr16:
#    .word 0
#    .word 0
# tmp_addr32:
#    .long 0x0
#    .word 0x0
# 32-bit code
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
    # Set up stack
#    mov %bp, saved_bp
#    mov %sp, saved_sp 
#    mov $RM_PROGRAM_STACK_PHYS_ADDR, %ebp
#    mov %ebp, %esp

    lgdt gdt_16_descriptor
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

    # Load 16-bit GDT
    # lgdt SWITCH_DATA_PHYS_ADDR - _data_start + gdt_16_descriptor
    # movl SWITCH_DATA_PHYS_ADDR, %eax
    # subl  _data_start, %eax
    # addl  gdt_16_descriptor, %eax
    # movl %eax, tmp_addr32_ptr
    # addw $0x8, tmp_addr32_ptr+4
    # mov tmp_addr32_ptr, %eax
    # lgdt (%eax)

    # Far jump to 16-bit protected mode
    # ljmp $0x08, (SWITCH_TEXT_PHYS_ADDR - _data_start + $1f)


    # Disable protected mode
    mov %cr0, %eax
    and $0xFFFFFFFE, %eax    # Clear PE bit
    mov %eax, %cr0

    # Far jump to real mode
    # ljmp $0x0, SWITCH_DATA_PHYS_ADDR - _data_start + $2f

ljmp $0x0000, $2f
2:

# 16-bit code
.code16

    # Load real mode data segments
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss



    # Set up stack
#    mov %bp, saved_bp
#    mov %sp, saved_sp 
    mov $RM_PROGRAM_STACK_PHYS_ADDR, %bp
    mov %bp, %sp

    # Load real mode IDT
    # lidt SWITCH_DATA_PHYS_ADDR - _data_start + real_mode_idt
    # mov SWITCH_DATA_PHYS_ADDR, %ax
    # sub  _data_start, %ax
    # add  real_mode_idt, %ax
    # mov %ax, tmp_addr16_ptr
    # add $0x0, tmp_addr16_ptr+2
    # mov tmp_addr16_ptr, %ax
    # lidt tmp_addr16_ptr
    lidt real_mode_idt
    # Enable interrupts
    sti

#    call 0x2000

    mov $0x0E41, %ax        # AH=0x0E (teletype), AL='A'
    mov $0x0017, %bx        # Page 0, no color
    int $0x10


    mov %ax, program_return_value

    # call SWITCH_DATA_PHYS_ADDR - _data_start + return_to_protected_mode
    # movl SWITCH_TEXT_PHYS_ADDR, %eax
    # subl  _text_start, %eax
    # addl  return_to_protected_mode, %eax
    # mov %ax, tmp_addr16_ptr
    # add $0x0, tmp_addr16_ptr+2
    # mov tmp_addr16_ptr, %ax
    # call tmp_addr16_ptr



    # Enable protected mode
    cli

    lgdt tmpgdt_32_descriptor

    mov %cr0, %eax
    or $0x1, %eax           # Set PE bit
    mov %eax, %cr0

    # Far jump to 32-bit protected mode
    ljmp $0x08, $3f
3:

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

    ljmp $0x08, $4f # + KERNEL_VIRT_BASE
4:
# call return_to_protected_mode

    # Reload original GDT (assuming it's at 0x1000)

    lgdt saved_gdt
    ljmp $0x08, $5f # + KERNEL_VIRT_BASE
5:

    lidt saved_idt

    ljmp $0x08, $6f
6:

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

#    mov $saved_bp, %bp
#    mov $saved_sp, %sp

    # end of *enter_real_mode_ptr

#    movl saved_esp, %esp
#    movl saved_ebp, %ebp

#    end of call _run_rm_program

    popl %eax
#    movl %ebp, %esp
    popl %ebp
    ret


_text_end:

.section .data
.align 16
_data_start:
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
.align 16
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

.align 16
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

.align 16
gdt_16_descriptor: 
    .word 0x17          # Size of GDT - 1
    .long gdt_16        # Address of GDT

.align 16
tmpgdt_32_descriptor: 
    .word 0x17          # Size of GDT - 1
    .long tmpgdt_32        # Address of GDT

real_mode_idt: 
    .word 0x3FF         # Limit
    .long 0x0           # Base
program_return_value:
    .word 0
_data_end: