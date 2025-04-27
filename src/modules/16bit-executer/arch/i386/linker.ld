OUTPUT_FORMAT(binary)
ENTRY(_mstart)                /* Entry point symbol for the kernel */
SECTIONS
{
    . = 0x1000;
        _text_start2 = .;
    .text : {
        *(.text)
    }
        _text_end2 = .;
        _data_start2 = .;
    .data : {
        *(.data)
    }
        _data_end2 = .;
    .bss : { *(.bss) }
    /DISCARD/ : { *(*) }
}