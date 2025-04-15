.global switch_to_process_asm
switch_to_process_asm:
    cli
    mov 4(%esp), %eax       # Load regs pointer
    mov 0(%eax), %ebx       # eax
    mov 4(%eax), %ecx       # ebx
    mov 8(%eax), %edx       # ecx
    mov 12(%eax), %esi      # edx
    mov 16(%eax), %edi      # esi
    mov 20(%eax), %ebp      # edi
    mov 40(%eax), %ebx      # ds
    mov %bx, %ds
    mov %bx, %es
    mov 48(%eax), %ebx      # fs
    mov %bx, %fs
    mov 52(%eax), %ebx      # gs
    mov %bx, %gs
    mov 56(%eax), %ebx      # ss
    mov %bx, %ss
    mov 28(%eax), %esp      # esp
    push 56(%eax)           # ss
    push 28(%eax)           # esp
    push 60(%eax)           # eflags
    push 36(%eax)           # cs
    push 32(%eax)           # eip
    mov 24(%eax), %eax      # ebp -> eax
    iret

.global schedule_asm
schedule_asm:
    cli                     # Disable interrupts
    mov 4(%esp), %eax       # Load regs pointer from stack
    mov 0(%eax), %ebx       # eax
    mov 4(%eax), %ecx       # ebx
    mov 8(%eax), %edx       # ecx
    mov 12(%eax), %esi      # edx
    mov 16(%eax), %edi      # esi
    mov 20(%eax), %ebp      # edi
    mov 40(%eax), %ebx      # ds
    mov %bx, %ds
    mov %bx, %es
    mov 48(%eax), %ebx      # fs
    mov %bx, %fs
    mov 52(%eax), %ebx      # gs
    mov %bx, %gs
    mov 56(%eax), %ebx      # ss
    mov %bx, %ss
    mov 28(%eax), %esp      # esp
    push 56(%eax)           # Push ss
    push 28(%eax)           # Push esp
    push 60(%eax)           # Push eflags
    push 36(%eax)           # Push cs
    push 32(%eax)           # Push eip
    mov 24(%eax), %eax      # Load ebp into eax (becomes eax after iret)
    iret                    # Restore context