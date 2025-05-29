#include <kernel/binfmt/elf.h>
#include <kernel/utils/kmem.h>
#include <string.h>

// Validate ELF header
int validate_elf(elf_header_t *header) {
    if (header->e_ident[0] != 0x7F || header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' || header->e_ident[3] != 'F') {
        return -1; // Not an ELF file
    }
    if (header->e_type != 2 || header->e_machine != 3 || header->e_version != 1) {
        return -2; // Invalid type or architecture
    }
    return 0;
}

// Load ELF file into process memory
int load_elf(const char *absPath, elf_info_t *elf_info) {
    uint8_t *elf_data;
    uint32_t elf_size;
    if (loadFile(absPath, &elf_data, &elf_size) != 0) {
        return -1; // Failed to load file
    }

    elf_header_t *header = (elf_header_t *)elf_data;
    if (validate_elf(header) != 0) {
        kfree(elf_data);
        return -2; // Invalid ELF
    }

    elf_info->num_segments = header->e_phnum;
    elf_info->segments = kmalloc(header->e_phnum * sizeof(uint32_t));
    elf_info->phys_addrs = kmalloc(header->e_phnum * sizeof(uint32_t));
    elf_info->sizes = kmalloc(header->e_phnum * sizeof(uint32_t));
    elf_info->entry_point = header->e_entry;

    elf_program_header_t *ph = (elf_program_header_t *)(elf_data + header->e_phoff);
    for (uint32_t i = 0; i < header->e_phnum; i++) {
        if (ph[i].p_type == 1) { // PT_LOAD
            uint32_t phys_addr = allocate_physical_pages(ph[i].p_memsz);
            if (!phys_addr) {
                kfree(elf_info->segments);
                kfree(elf_info->phys_addrs);
                kfree(elf_info->sizes);
                kfree(elf_data);
                return -3; // Out of memory
            }

            map_pages(current_process->page_dir, ph[i].p_vaddr, phys_addr, ph[i].p_memsz, 
                      USER_READ_WRITE | (ph[i].p_flags & 0x1 ? USER_EXEC : 0));

            memcpy((void *)phys_addr, elf_data + ph[i].p_offset, ph[i].p_filesz);
            if (ph[i].p_memsz > ph[i].p_filesz) {
                memset((void *)(phys_addr + ph[i].p_filesz), 0, ph[i].p_memsz - ph[i].p_filesz);
            }

            elf_info->segments[i] = ph[i].p_vaddr;
            elf_info->phys_addrs[i] = phys_addr;
            elf_info->sizes[i] = ph[i].p_memsz;
        }
    }

    kfree(elf_data);
    return 0;
}