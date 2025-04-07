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

ISR_STUB 0, 0   # Divide error
ISR_STUB 1, 0   # Debug
ISR_STUB 14, 1  # Page fault
ISR_STUB 32, 0  # Timer
ISR_STUB 33, 0  # Keyboard
# Extend as needed