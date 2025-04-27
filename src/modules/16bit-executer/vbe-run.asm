[BITS 16]
    ; Print character using BIOS interrupt
    mov ax, [0x600A]
    mov si, ax
    mov ax, [0x6000]
    mov bx, [0x6002]
    mov cx, [0x6004]
    mov dx, [0x6006]
    mov di, [0x6008]
    int 0x10                ; BIOS video interrupt
    ret