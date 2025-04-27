; vbe_bios.asm
; Assemble with: nasm -f elf32 vbe_bios.asm -o vbe_bios.o
section .text
global vbe_bios_call

vbe_bios_call:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    ; Parameters: ax, bx, cx, dx, es_di
    mov eax, [ebp + 8]  ; AX
    mov ebx, [ebp + 12] ; BX
    mov ecx, [ebp + 16] ; CX
    mov edx, [ebp + 20] ; DX
    mov edi, [ebp + 24] ; ES:DI (DI only, ES set below)

    ; Copy real-mode code to low memory (e.g., 0x1000)
    mov esi, real_mode_code
    mov edi, 0x1000
    mov ecx, real_mode_code_end - real_mode_code
    cld
    rep movsb

    ; Set up real-mode segment registers
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Switch to real mode (simplified, assumes GDT/IDT setup)
    ; Save protected mode state
    push ds
    push es
    push fs
    push gs
    cli
    mov [0x2000], esp  ; Save stack pointer

    ; Load real-mode IDT
    lidt [real_mode_idt]

    ; Disable protected mode
    mov eax, cr0
    and eax, ~1
    mov cr0, eax

    ; Far jump to flush prefetch queue
    jmp 0x0000:real_mode_entry

real_mode_entry:
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00  ; Real-mode stack

    ; Set up registers for INT 0x10
    mov ax, [ebp + 8]  ; AX
    mov bx, [ebp + 12] ; BX
    mov cx, [ebp + 16] ; CX
    mov dx, [ebp + 20] ; DX
    mov di, [ebp + 24] ; DI
    mov es, [ebp + 24 + 2] ; ES

    ; Call INT 0x10
    int 0x10

    ; Save return value (AX)
    mov [0x2004], ax

    ; Switch back to protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Restore segment registers
    mov ax, 0x10  ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, [0x2000]

    ; Restore IDT
    lidt [protected_mode_idt]

    ; Return AX
    mov eax, [0x2004]

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

section .data
real_mode_idt:
    dw 0x3FF  ; IDT limit
    dd 0      ; IDT base

protected_mode_idt:
    dw 0      ; Set by OS
    dd 0      ; Set by OS

real_mode_code:
    ; Placeholder for real-mode code
    int 0x10
    retf
real_mode_code_end: