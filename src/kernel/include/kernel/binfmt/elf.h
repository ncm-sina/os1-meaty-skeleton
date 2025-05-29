#ifndef ELF_H
#define ELF_H

#include <stdint.h>

// ELF-specific binary info
typedef struct {
    uint32_t entry_point; // ELF e_entry (_start address)
    uint32_t* segments;   // Array of segment virtual addresses
    uint32_t* phys_addrs; // Corresponding physical addresses
    uint32_t* sizes;      // Segment sizes
    uint32_t num_segments;// Number of loadable segments
} __attribute__((packed)) elf_info_t;

// ELF header (simplified for 32-bit)
typedef struct {
    uint8_t e_ident[16];  // ELF identification
    uint16_t e_type;      // Object file type
    uint16_t e_machine;   // Machine architecture
    uint32_t e_version;   // Object file version
    uint32_t e_entry;     // Entry point virtual address
    uint32_t e_phoff;     // Program header offset
    uint32_t e_shoff;     // Section header offset
    uint32_t e_flags;     // Processor-specific flags
    uint16_t e_ehsize;    // ELF header size
    uint16_t e_phentsize; // Program header entry size
    uint16_t e_phnum;     // Number of program headers
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t p_type;   // Segment type (e.g., PT_LOAD = 1)
    uint32_t p_offset; // Offset in file
    uint32_t p_vaddr;  // Virtual address
    uint32_t p_paddr;  // Physical address (ignored for user space)
    uint32_t p_filesz; // Size in file
    uint32_t p_memsz;  // Size in memory
    uint32_t p_flags;  // Segment flags (e.g., read, write, execute)
    uint32_t p_align;  // Alignment
} __attribute__((packed)) elf_program_header_t;

#endif