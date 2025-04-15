# Specify 32-bit mode
.code32

# Section for code
.section .text

# Declare global symbols
.global is_A20_on

is_A20_on:
    pushal              # Push all 32-bit general registers (equivalent to pushad)
    movl $0x112345, %edi # Odd megabyte address
    movl $0x012345, %esi # Even megabyte address
    movl %esi, (%esi)   # Store ESI at address pointed by ESI
    movl %edi, (%edi)   # Store EDI at address pointed by EDI
    cmpsl               # Compare 32-bit values at ESI and EDI (equivalent to cmpsd)
    popal               # Pop all 32-bit general registers (equivalent to popad)
    jne A20_on          # Jump if not equal (A20 is on)
    movl $0x0, %eax
.done:
    ret                 # Return if equal (A20 is off)

A20_on:
    movl $0x1, %eax
    jmp .done
    # Your code would go here
    