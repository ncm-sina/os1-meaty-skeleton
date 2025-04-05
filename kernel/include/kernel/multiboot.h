#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* Multiboot header magic */
#define MULTIBOOT_HEADER_MAGIC  0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* Flags for multiboot_info_t */
#define MULTIBOOT_INFO_MEMORY   (1 << 0)
#define MULTIBOOT_INFO_MEM_MAP  (1 << 6)

/* Memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE  1
#define MULTIBOOT_MEMORY_RESERVED   2
#define MULTIBOOT_MEMORY_ACPI       3
#define MULTIBOOT_MEMORY_NVS        4
#define MULTIBOOT_MEMORY_BADRAM     5

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
} multiboot_header_t;

/* Multiboot info structure (runtime, post-boot) */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;    /* KB below 1 MB */
    uint32_t mem_upper;    /* KB above 1 MB */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;  /* Length of memory map */
    uint32_t mmap_addr;    /* Address of memory map */
} multiboot_info_t;

/* Memory map entry */
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;         /* 1 = available RAM */
} multiboot_memory_map_t;

#endif