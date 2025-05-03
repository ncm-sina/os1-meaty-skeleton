#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* Multiboot header magic */
#define MULTIBOOT_HEADER_MAGIC  0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002


// Flags for multiboot_info_t
#define MULTIBOOT_INFO_MEMORY    (1 << 0)  // 0x1: mem_lower, mem_upper valid
#define MULTIBOOT_INFO_BOOTDEV   (1 << 1)  // 0x2: boot_device valid
#define MULTIBOOT_INFO_CMDLINE   (1 << 2)  // 0x4: cmdline valid
#define MULTIBOOT_INFO_MODS      (1 << 3)  // 0x8: mods_count, mods_addr valid
#define MULTIBOOT_INFO_AOUT_SYMS (1 << 4)  // 0x10: a.out symbol table
#define MULTIBOOT_INFO_ELF_SHDR  (1 << 5)  // 0x20: ELF section headers
#define MULTIBOOT_INFO_MEM_MAP   (1 << 6)  // 0x40: mmap_length, mmap_addr valid
#define MULTIBOOT_INFO_DRIVE_INFO (1 << 7) // 0x80: drives_length, drives_addr
#define MULTIBOOT_INFO_CONFIG_TABLE (1 << 8) // 0x100: config table
#define MULTIBOOT_INFO_BOOT_LOADER_NAME (1 << 9) // 0x200: bootloader name
#define MULTIBOOT_INFO_APM_TABLE (1 << 10) // 0x400: APM table
#define MULTIBOOT_INFO_VIDEO_INFO (1 << 11) // 0x800: video info

/* Memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE  1 // Usable RAM
#define MULTIBOOT_MEMORY_RESERVED   2 // Reserved, not usable
#define MULTIBOOT_MEMORY_ACPI       3 // ACPI reclaimable memory
#define MULTIBOOT_MEMORY_NVS        4 // Non-volatile storage (e.g., ACPI NVS)
#define MULTIBOOT_MEMORY_BADRAM     5 // Defective RAM, not usable

/* Multiboot header structure (static, pre-boot) */
typedef struct {
    uint32_t magic;        /* Must be 0x1BADB002 */
    uint32_t flags;        /* Feature flags */
    uint32_t checksum;     /* MAGIC + FLAGS + CHECKSUM = 0 */
    uint32_t header_addr;  /* If flags[16] set */
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
    uint32_t mode_type;    /* If flags[2] set (video) */
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} __attribute__((packed)) multiboot_header_t;

// Memory map entry
typedef struct {
    uint32_t size;       // Size of this structure (including itself)
    uint64_t addr;       // Starting address of memory region
    uint64_t len;        // Length of memory region
    uint32_t type;       // 1 = available, 2 = reserved, etc.
} __attribute__((packed)) multiboot_memory_map_t;

// Module entry
typedef struct {
    uint32_t mod_start;  // Physical address of module start
    uint32_t mod_end;    // Physical address of module end
    uint32_t cmdline;    // Physical address of command line string
    uint32_t reserved;   // Must be 0
} __attribute__((packed)) multiboot_module_t;

// Multiboot info structure (passed by bootloader)
typedef struct {
    uint32_t flags;            // Flags indicating available fields
    uint32_t mem_lower;        // Memory below 1MB (in KB)
    uint32_t mem_upper;        // Memory above 1MB (in KB)
    uint32_t boot_device;      // Boot device info
    uint32_t cmdline;          // Physical address of command line
    uint32_t mods_count;       // Number of loaded modules
    uint32_t mods_addr;        // Physical address of module array
    uint32_t syms[4];          // Symbol table info (a.out or ELF)
    uint32_t mmap_length;      // Length of memory map buffer
    uint32_t mmap_addr;        // Physical address of memory map
    uint32_t drives_length;    // Length of drives info
    uint32_t drives_addr;      // Physical address of drives info
    uint32_t config_table;     // ROM configuration table
    uint32_t boot_loader_name; // Physical address of bootloader name
    uint32_t apm_table;        // APM table address
    uint32_t vbe_control_info; // VBE control info
    uint32_t vbe_mode_info;    // VBE mode info
    uint16_t vbe_mode;         // Current VBE mode
    uint16_t vbe_interface_seg;// VBE interface segment
    uint16_t vbe_interface_off;// VBE interface offset
    uint16_t vbe_interface_len;// VBE interface length
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} __attribute__((packed)) multiboot_info_t;

extern multiboot_info_t *_mbi;

#endif