; switch_to_realmode_with_paging.asm
; Switches from 32-bit protected mode (with paging) to 16-bit real mode,
; restores IVT, and uses BIOS interrupt to print a character
global _mstart

[BITS 32]
_mstart:
    ; Ensure protected mode segment registers
    push ebp
    mov ebp, esp

; step 0 save stuff
    sgdt [_gdt_descriptor]
    sidt [_idt_descriptor]

    mov eax, cr0
    mov [_cr0], eax

    mov eax, cr3
    mov [_cr3], eax

    mov eax, esp
    mov [_esp], eax

    mov eax, ebp
    mov [_ebp], eax


    mov ax, DATA32_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, RM_STACK_TOP        ; Stack pointer

    ; Step 1: Disable interrupts
    cli


    ; Step 2: Disable paging
    mov eax, cr0
    and eax, ~0x80000000    ; Clear PG bit
    mov cr0, eax

    xor eax, eax
    mov cr3, eax

    ; Step 3: Load GDT with 16-bit segments
    lgdt [gdt_descriptor]

    ; Step 4: Switch to 16-bit code segment
    jmp CODE16_SEG:switch_to_16bit
switch_to_16bit:

[BITS 16]
    ; Update segment registers for 16-bit operation
    mov ax, DATA16_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; mov sp, RM_STACK_TOP          ; Stack pointer for real mode

    ; Step 5: Disable protected mode
    mov eax, cr0
    and eax, ~1             ; Clear PE bit
    mov cr0, eax

    ; Step 6: Far jump to real mode
    jmp 0x0000:real_mode

real_mode:
    ; Set real-mode segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; mov sp, RM_STACK_TOP

    ; Set up segment registers
    xor ax, ax          ; AX = 0
    mov ds, ax          ; Data segment = 0
    mov es, ax          ; Extra segment = 0

    ; Step 7: Restore real-mode IVT
    lidt [ivt_descriptor]

    ; Step 8: Re-enable interrupts for BIOS
    sti

    ; Step 9: call function
    call 0x2000

    mov [_ret_val], ax
    mov bx, 0
    call print_h16

    cli

    ; Step 10: enable protected mode and paging


    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax
    jmp CODE32_SEG:protected_mode
protected_mode:


[BITS 32]
    mov ax, DATA32_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov edi, VGA_BUFFER + 0xA0     ; VGA text buffer address
    mov al, '1'             ; Character to print
    mov ah, 0x0F            ; Attribute: white text on black background
    mov [edi], ax           ; Write character + attribute


    mov eax, [_cr3]
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp CODE32_SEG:enabled_paging
enabled_paging:

    mov edi, VGA_BUFFER + 0xA2     ; VGA text buffer address
    mov al, '2'             ; Character to print
    mov ah, 0x0F            ; Attribute: white text on black background
    mov [edi], ax           ; Write character + attribute


    lidt [_idt_descriptor]
    jmp CODE32_SEG:enabled_idt
enabled_idt:

    mov edi, VGA_BUFFER + 0xA4     ; VGA text buffer address
    mov al, '3'             ; Character to print
    mov ah, 0x0F            ; Attribute: white text on black background
    mov [edi], ax           ; Write character + attribute


    sti

    mov ebp, [_esp]
    mov esp, ebp
    pop ebp
    ret

; 16-bit macro to print AX as 4 hex digits at (x, y)
; [BITS 16]
; %macro print_hex_16 2
;     pusha
;     mov bx, ax             ; Copy AX to BX
;     mov cx, 4              ; 4 hex digits
;     ; Calculate VGA offset: (y * 80 + x) * 2
;     mov ax, %2             ; y
;     mov dx, 80
;     mul dx                 ; AX = y * 80
;     add ax, %1             ; AX = y * 80 + x
;     shl ax, 1              ; AX = (y * 80 + x) * 2
;     mov di, ax             ; DI = VGA offset
;     mov ax, 0xB800
;     mov es, ax
;     ; Print "0x"
;     mov word [es:di], 0x0F30 ; '0'
;     add di, 2
;     mov word [es:di], 0x0F78 ; 'x'
;     add di, 2
; %%loop:
;     rol bx, 4
;     mov al, bl
;     and al, 0x0F
;     cmp al, 0x0A
;     jb %%digit
;     add al, 0x07
; %%digit:
;     add al, 0x30
;     mov ah, 0x0F
;     mov [es:di], ax
;     add di, 2
;     loop %%loop
;     popa
; %endmacro

; 32-bit macro to print EAX as 8 hex digits at (x, y)
; [BITS 32]
; %macro print_hex_32 2
;     pusha
;     mov ebx, eax           ; Copy EAX to EBX
;     mov ecx, 8             ; 8 hex digits
;     ; Calculate VGA offset: (y * 80 + x) * 2
;     mov eax, %2            ; y
;     mov edx, 80
;     mul edx                ; EAX = y * 80
;     add eax, %1            ; EAX = y * 80 + x
;     shl eax, 1             ; EAX = (y * 80 + x) * 2
;     mov edi, eax           ; EDI = VGA offset
;     mov ax, 0xB800
;     mov es, ax
;     ; Print "0x"
;     mov word [es:edi], 0x0F30 ; '0'
;     add edi, 2
;     mov word [es:edi], 0x0F78 ; 'x'
;     add edi, 2
; %%loop:
;     rol ebx, 4
;     mov al, bl
;     and al, 0x0F
;     cmp al, 0x0A
;     jb %%digit
;     add al, 0x07
; %%digit:
;     add al, 0x30
;     mov ah, 0x0F
;     mov [es:edi], ax
;     add edi, 2
;     loop %%loop
;     popa
; %endmacro

[BITS 16]
; 16-bit function to print AX as 4 hex digits at (x=BH, y=BL)
print_h16:
    pusha                  ; Save all registers
    push es                ; Save ES
    mov si, ax             ; Copy AX to SI for processing
    mov cx, 4              ; 4 hex digits

    ; Calculate VGA offset: (y * 80 + x) * 2
    xor ax, ax
    mov al, bl             ; y
    mov dx, 80
    mul dx                 ; AX = y * 80
    mov bl, bh             ; x (BH to BL)
    mov bh, 0
    add ax, bx             ; AX = y * 80 + x
    shl ax, 1              ; AX = (y * 80 + x) * 2
    mov di, ax             ; DI = VGA offset

    mov ax, 0xB800         ; VGA text buffer
    mov es, ax

    ; Print "0x"
    mov word [es:di], 0x0F30 ; '0' (white on black)
    add di, 2
    mov word [es:di], 0x0F78 ; 'x'
    add di, 2

.print_loop:
    rol si, 4              ; Rotate left 4 bits
    mov ax, si             ; Get SI (instead of SIL)
    and ax, 0x0F           ; Mask to lowest 4 bits
    cmp al, 0x0A           ; If < 10, add '0'
    jb .digit
    add al, 0x07           ; Else add 'A'-10
.digit:
    add al, 0x30           ; Convert to ASCII
    mov ah, 0x0F           ; White on black
    mov [es:di], ax        ; Write to VGA
    add di, 2              ; Next position
    loop .print_loop

    pop es                 ; Restore ES
    popa                   ; Restore registers
    ret

[BITS 32]
; 32-bit function to print EAX as 8 hex digits at (x=BH, y=BL)
print_h32:
    pusha                  ; Save all registers
    push es                ; Save ES
    mov esi, eax           ; Copy EAX to ESI
    mov ecx, 8             ; 8 hex digits

    ; Calculate VGA offset: (y * 80 + x) * 2
    xor eax, eax
    mov al, bl             ; y
    mov edx, 80
    mul edx                ; EAX = y * 80
    xor ebx, ebx
    mov bl, bh             ; x (BH to BL)
    add eax, ebx           ; EAX = y * 80 + x
    shl eax, 1             ; EAX = (y * 80 + x) * 2
    mov edi, eax           ; EDI = VGA offset

    mov ax, 0xB800         ; VGA text buffer
    mov es, ax

    ; Print "0x"
    mov word [es:edi], 0x0F30 ; '0'
    add edi, 2
    mov word [es:edi], 0x0F78 ; 'x'
    add edi, 2

.print_loop:
    rol esi, 4             ; Rotate left 4 bits
    mov eax, esi            ; Get ESI (instead of SIL)
    and eax, 0x0F           ; Mask to lowest 4 bits
    cmp al, 0x0A           ; If < 10, add '0'
    jb .digit
    add al, 0x07           ; Else add 'A'-10
.digit:
    add al, 0x30           ; Convert to ASCII
    mov ah, 0x0F           ; White on black
    mov [es:edi], ax       ; Write to VGA
    add edi, 2             ; Next position
    loop .print_loop

    pop es                 ; Restore ES
    popa                   ; Restore registers
    ret

; print_h16:
;     print_hex_16 0,0
;     ret

; print_h32:
;     print_hex_32 1,1
;     ret



; data
_cr0:       dd 0
_cr3:       dd 0
_esp:       dd 0
_ebp:       dd 0
_ret_val:   dw 0
_gdt_descriptor:
    dw 0 
    dd 0
_idt_descriptor:
    dw 0 
    dd 0

; IVT descriptor for real mode
ivt_descriptor:
    dw 0x03FF               ; Limit: 1KB (256 interrupts * 4 bytes)
    dd 0x00000000           ; Base: 0x0000 (IVT at 0x0000:0x0000)

; GDT setup
gdt_start:
    ; Null descriptor
    dd 0x00000000
    dd 0x00000000
    ; 32-bit code segment (base=0, limit=4GB, executable, 32-bit)
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x9A         ; Access: present, privilege=0, code, executable, readable
    db 0xCF         ; Flags: 4KB granularity, 32-bit, limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)
    ; 32-bit data segment (base=0, limit=4GB, writable)
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x92         ; Access: present, privilege=0, data, writable
    db 0xCF         ; Flags: 4KB granularity, 32-bit, limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)
    ; 16-bit code segment (base=0, limit=64KB, executable, 16-bit)
    dw 0xFFFF       ; Limit (bits 0-15) = 64KB
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x9A         ; Access: present, privilege=0, code, executable, readable
    db 0x0F         ; Flags: byte granularity, 16-bit, limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)
    ; 16-bit data segment (base=0, limit=64KB, writable)
    dw 0xFFFF       ; Limit (bits 0-15) = 64KB
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x92         ; Access: present, privilege=0, data, writable
    db 0x0F         ; Flags: byte granularity, 16-bit, limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size
    dd gdt_start                ; GDT address

; Segment selectors
CODE32_SEG equ 0x08     ; 32-bit code segment
DATA32_SEG equ 0x10     ; 32-bit data segment
CODE16_SEG equ 0x18     ; 16-bit code segment
DATA16_SEG equ 0x20     ; 16-bit data segment

VGA_BUFFER equ 0xB8000     ; 16-bit data segment
RM_STACK_TOP equ 0x6FF0     ; 16-bit data segment
