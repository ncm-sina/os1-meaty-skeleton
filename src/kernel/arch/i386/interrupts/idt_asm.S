.section .text

.globl idt_load
.type idt_load, @function
idt_load:
    movl 4(%esp), %eax
    lidt (%eax)
    ret

.extern isr_handler

.macro ISR_STUB num, has_error=0
    .globl isr\num
    .type isr\num, @function
    isr\num:
        pusha
        .if \has_error == 0
            pushl $0    # Dummy error code
        .endif
        pushl $\num     # Vector number
        call isr_handler
        addl $8, %esp   # Pop vector and error code
        popa
        iret
.endm

.globl isr_default
.type isr_default, @function
isr_default:
    pusha
    pushl $0
    pushl $-1       # Default vector
    call isr_handler
    addl $8, %esp
    popa
    iret

# ISR stubs for vectors 0-47
ISR_STUB 0, 0   # Divide Error
ISR_STUB 1, 0   # Debug
ISR_STUB 2, 0   # Non-Maskable Interrupt (NMI)
ISR_STUB 3, 0   # Breakpoint
ISR_STUB 4, 0   # Overflow
ISR_STUB 5, 0   # Bound Range Exceeded
ISR_STUB 6, 0   # Invalid Opcode
ISR_STUB 7, 0   # Device Not Available
ISR_STUB 8, 1   # Double Fault (error code)
ISR_STUB 9, 0   # Coprocessor Segment Overrun (legacy, no error code)
ISR_STUB 10, 1  # Invalid TSS (error code)
ISR_STUB 11, 1  # Segment Not Present (error code)
ISR_STUB 12, 1  # Stack-Segment Fault (error code)
ISR_STUB 13, 1  # General Protection Fault (error code)
ISR_STUB 14, 1  # Page Fault (error code)
ISR_STUB 15, 0  # Reserved
ISR_STUB 16, 0  # x87 FPU Floating-Point Error
ISR_STUB 17, 1  # Alignment Check (error code)
ISR_STUB 18, 0  # Machine Check
ISR_STUB 19, 0  # SIMD Floating-Point Exception
ISR_STUB 20, 0  # Virtualization Exception
ISR_STUB 21, 0  # Control Protection Exception
ISR_STUB 22, 0  # Reserved
ISR_STUB 23, 0  # Reserved
ISR_STUB 24, 0  # Reserved
ISR_STUB 25, 0  # Reserved
ISR_STUB 26, 0  # Reserved
ISR_STUB 27, 0  # Reserved
ISR_STUB 28, 0  # Hypervisor Injection Exception
ISR_STUB 29, 0  # VMM Communication Exception
ISR_STUB 30, 0  # Security Exception
ISR_STUB 31, 0  # Reserved
ISR_STUB 32, 0  # IRQ0: Timer
ISR_STUB 33, 0  # IRQ1: Keyboard
ISR_STUB 34, 0  # IRQ2: Cascade (PIC internal)
ISR_STUB 35, 0  # IRQ3: COM2
ISR_STUB 36, 0  # IRQ4: COM1
ISR_STUB 37, 0  # IRQ5: LPT2
ISR_STUB 38, 0  # IRQ6: Floppy Disk
ISR_STUB 39, 0  # IRQ7: LPT1
ISR_STUB 40, 0  # IRQ8: RTC
ISR_STUB 41, 0  # IRQ9: Free / ACPI
ISR_STUB 42, 0  # IRQ10: Free
ISR_STUB 43, 0  # IRQ11: Free
ISR_STUB 44, 0  # IRQ12: PS/2 Mouse
ISR_STUB 45, 0  # IRQ13: FPU / Coprocessor
ISR_STUB 46, 0  # IRQ14: Primary IDE
ISR_STUB 47, 0  # IRQ15: Secondary IDE
# Extend as needed