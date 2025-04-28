[BITS 16]
    ; Print character using BIOS interrupt
    mov ax, [0x600A]
    mov es, ax
    mov ax, [0x6000]
    mov bx, [0x6002]
    mov cx, [0x6004]
    mov dx, [0x6006]
    mov di, [0x6008]
    int 0x10                ; BIOS video interrupt

    mov [0x6000] , ax
    mov [0x6002] , bx
    mov [0x6004] , cx
    mov [0x6006] , dx
    mov [0x6008] , di
    mov ax, es
    mov [0x600A] , ax
    mov ax, [0x6000]
    ret