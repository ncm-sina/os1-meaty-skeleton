.global syscall_handler
.extern syscall_dispatch

syscall_handler:
    pusha                # Save all general-purpose registers
    pushl %ebp           # Save base pointer
    pushl %edi           # Save destination index
    pushl %esi           # Save source index
    pushl %edx           # Save data register
    pushl %ecx           # Save counter register
    pushl %ebx           # Save base register
    call syscall_dispatch # Call C dispatcher
    addl $24, %esp       # Remove 6 pushed registers (6 * 4 bytes)
    popa                 # Restore general-purpose registers
    iret                 # Return from interrupt