# mode_switch.S
# Assembly code for mode switching

.set SWITCH_TEXT_PHYS_ADDR, 0x1000
.set SWITCH_DATA_PHYS_ADDR, 0x1F00
# .set SWITCH_STACK_PHYS_ADDR, 0x1FFF
.set RM_PROGRAM_TEXT_PHYS_ADDR, 0x02000
.set RM_PROGRAM_STACK_PHYS_ADDR, 0x07BFF
.set RM_PROGRAM_DATA_PHYS_ADDR, 0x06000

.set tmp_addr16_ptr, 0x0000
.set tmp_addr32_ptr, 0x0000

.set KERNEL_VIRT_BASE, 0xC0000000

# .extern pagedir
# .extern run_rm_program

.section .header
.global _header
_header:
    .long ._text_start  # Offset of .text
    .long ._text_end    # End of .text
    .long ._data_start  # Offset of .data
    .long ._data_end    # End of .data

.section .text
.global _mstart
.global enter_real_mode
.global _init_mode_switcher
.global _load_rm_program
.global _exec_rm_program
._text_start:
    jmp _init_mode_switcher

# Export run_rm_program address
.global run_rm_program_ptr
run_rm_program_ptr: .long _exec_rm_program

tmp_addr16:
    .word 0
    .word 0
tmp_addr32:
    .long 0
    .word 0
# 32-bit code
.code32
.switch_text_start:
_exec_rm_program:
    pushl %ebp
    movl %esp, %ebp
    pushl %eax

    # typedef struct {
    #  12(%ebp)   uint32_t entry_addr;
    #  16(%ebp)   uint32_t text_start_addr; // virtual address of binary start 
    #  20(%ebp)   uint16_t text_size; // virtual address of binary start
    #  22(%ebp)   uint32_t data_start_addr; // virtual address of binary start
    #  26(%ebp)   uint16_t data_size; // virtual address of binary start
    # } rm_binary_info_t;

    movl 12(%ebp), %eax
    movl %eax, _main_func_ptr

    movl 26(%ebp), %eax
    pushl %eax
    movl 22(%ebp), %eax
    pushl %eax
    movl 20(%ebp), %eax
    pushl %eax
    movl 16(%ebp), %eax
    pushl %eax
    movl 12(%ebp), %eax
    pushl %eax

    call _load_rm_program

    call _run_rm_program

    popl %eax
#    movl %ebp, %esp
    popl %ebp
    ret
_init_mode_switcher:
    # first we copy data and texts of switcher into their low memory 
    # addresses defined at top of this fil
    # here we assume we have identity map for PDE[0]

    # Copy switcher real-mode codes to 'SWITCH_TEXT_PHYS_ADDR'
    # mov $switch_text_start, %esi
    # mov $SWITCH_TEXT_PHYS_ADDR, %edi
    # mov $switch_text_end, %ecx
    # sub %esi, %ecx      # ecx = size of real-mode code
    # cld
    # rep movsb

    # # Copy switcher real-mode data to 'SWITCH_DATA_PHYS_ADDR'
    # mov $.switch_data_start, %esi
    # mov $SWITCH_DATA_PHYS_ADDR, %edi
    # mov $switch_data_end, %ecx
    # sub %esi, %ecx      # ecx = size of real-mode code
    # cld
    # rep movsb

    # store function address to be used elsewhere to run the function
    # mov enter_real_mode-switch_text_start+SWITCH_TEXT_PHYS_ADDR,enter_real_mode_rm
    # movl $SWITCH_TEXT_PHYS_ADDR, enter_real_mode_ptr
    
    ret

_load_rm_program:
    pushl %ebp
    movl %esp, %ebp

    pushl %eax
    pushl %ecx
    pushl %esi
    pushl %edi

    # Copy rm program text to 'RM_PROGRAM_TEXT_PHYS_ADDR'
    mov 12(%ebp), %esi
    movl 16(%ebp), %ecx
    mov $RM_PROGRAM_TEXT_PHYS_ADDR, %edi
    cld
    rep movsb

    # Copy rm program data to 'RM_PROGRAM_DATA_PHYS_ADDR' 
    mov 18(%ebp), %esi
    movl 22(%ebp), %ecx
    mov $RM_PROGRAM_DATA_PHYS_ADDR, %edi
    cld
    rep movsb

    movl 24(%ebp), %eax
    movl $RM_PROGRAM_TEXT_PHYS_ADDR, (%eax)

    popl %edi
    popl %esi
    popl %ecx
    popl %eax
    movl %ebp, %esp
    popl %ebp
    ret

_run_rm_program:
    pushl %ebp
    movl %esp, %ebp

#    mov 12(%ebp), %eax
#    mov %eax, _main_func_ptr # save function pointer (should be 0x2000)

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
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %esp
    pushl %ebp
    pushl %esi
    pushl %edi
    
    movl %esp, saved_esp
    movl %ebp, saved_ebp

    call *enter_real_mode_ptr

    movl saved_esp, %esp
    movl saved_ebp, %ebp

    popl %edi
    popl %esi
    popl %ebp
    popl %esp
    popl %ebx
    popl %edx
    popl %ecx

    movl %ebp, %esp
    popl %ebp

    ret


# 32-bit code
.code32
enter_real_mode:
    # Assume we're in a 1:1 mapped page
    # Disable paging
    mov %cr0, %eax
    and $0x7FFFFFFF, %eax    # Clear PG bit
    mov %eax, %cr0

    # Flush TLB
    mov $0, %eax
    mov %eax, %cr3

    # Load 16-bit GDT
    # lgdt SWITCH_DATA_PHYS_ADDR - .switch_data_start + gdt_16_descriptor
    movl SWITCH_DATA_PHYS_ADDR, %eax
    subl  .switch_data_start, %eax
    addl  gdt_16_descriptor, %eax
    movl %eax, tmp_addr32_ptr
    addw $0x8, tmp_addr32_ptr+4
    mov tmp_addr32_ptr, %eax
    lgdt (%eax)

    # Far jump to 16-bit protected mode
    # ljmp $0x08, (SWITCH_DATA_PHYS_ADDR - .switch_data_start + $1f)
    # movl SWITCH_DATA_PHYS_ADDR, %eax
    # subl  .switch_data_start, %eax
    # addl  $1f, %eax
    # ljmp $0x08, *%eax
    ljmp $0x08, $1f
1:
    # Load 16-bit data segments
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Disable protected mode
    mov %cr0, %eax
    and $0xFFFFFFFE, %eax    # Clear PE bit
    mov %eax, %cr0

    # Far jump to real mode
    # ljmp $0x0, SWITCH_DATA_PHYS_ADDR - .switch_data_start + $2f
    mov SWITCH_TEXT_PHYS_ADDR, %ax
    sub  .switch_text_start, %ax
    add  $2f, %ax
    mov %ax, tmp_addr16_ptr
    add $0x0, tmp_addr16_ptr+2
    mov tmp_addr16_ptr, %ax
    ljmp *tmp_addr16_ptr

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
    mov %bp, saved_bp
    mov %sp, saved_sp 
    mov $RM_PROGRAM_STACK_PHYS_ADDR, %bp
    mov %bp, %sp

    # Load real mode IDT
    # lidt SWITCH_DATA_PHYS_ADDR - .switch_data_start + real_mode_idt
    mov SWITCH_DATA_PHYS_ADDR, %ax
    sub  .switch_data_start, %ax
    add  real_mode_idt, %ax
    mov %ax, tmp_addr16_ptr
    add $0x0, tmp_addr16_ptr+2
    mov tmp_addr16_ptr, %ax
    lidt tmp_addr16_ptr

    # Enable interrupts
    # sti

    
    call _main_func_ptr

    push %ax

    # call SWITCH_DATA_PHYS_ADDR - .switch_data_start + return_to_protected_mode
    movl SWITCH_TEXT_PHYS_ADDR, %eax
    subl  .switch_text_start, %eax
    addl  return_to_protected_mode, %eax
    mov %ax, tmp_addr16_ptr
    add $0x0, tmp_addr16_ptr+2
    mov tmp_addr16_ptr, %ax
    call tmp_addr16_ptr

    pop %ax

    mov $saved_bp, %bp
    mov $saved_sp, %sp

    ret

.global return_to_protected_mode
return_to_protected_mode:
    # Disable interrupts
    cli

    # Reload original GDT (assuming it's at 0x1000)
    lgdt saved_gdt

    # Enable protected mode
    mov %cr0, %eax
    or $0x1, %eax           # Set PE bit
    mov %eax, %cr0

    # Far jump to 32-bit protected mode
    ljmp $0x08, $3f

3:

    # Reload segment registers
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Re-enable paging
    mov saved_cr3, %eax     # Assuming page directory at 1MB
    mov %eax, %cr3
    mov %cr0, %eax
    or $0x80000000, %eax    # Set PG bit
    mov %eax, %cr0

    ljmp $0x08, $4f # + KERNEL_VIRT_BASE
4:
    lidt saved_idt

    # Restore interrupt state
    mov $1, %eax
    cmp $is_int_enabled, %eax
    jne .no_sti
    sti                 # Re-enable interrupts if previously enabled
    jmp .continue2
.no_sti:
    cli
.continue2:

    ret
.switch_text_end:
._text_end:

.section .data
.switch_data_start:
._data_start:
saved_esp: .long 0
saved_ebp: .long 0
saved_gdt: .space 6
saved_idt: .space 6
saved_cr3: .long 0
is_int_enabled: .word 0
enter_real_mode_ptr:
.long 0
_main_func_ptr:
.long 0
saved_sp: .word 0
saved_bp: .word 0
# 16-bit GDT
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

gdt_16_descriptor:
    .word 0x17          # Size of GDT - 1
    .long gdt_16        # Address of GDT

real_mode_idt:
    .word 0x3FF         # Limit
    .long 0x0           # Base
.switch_data_end:
._data_end: