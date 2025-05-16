# IRQ14 handler for IDE primary channel
.section .text
.global irq14_handler
.extern irq_dispatch

irq14_handler:
    pusha                    # Save all registers
    push $14                 # Push IRQ number
    call irq_dispatch        # Call C dispatcher
    add $4, %esp            # Clean up stack
    popa                     # Restore registers
    iret                     # Return from interrupt