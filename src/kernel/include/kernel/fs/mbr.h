#ifndef MBR_H
#define MBR_H

#include <stdint.h>
#include <kernel/drivers/ide.h>

// Partition entry structure (16 bytes)
struct mbr_partition {
    uint8_t status;           // 0x80 (bootable), 0x00 (non-bootable)
    uint8_t first_chs[3];     // Ignored
    uint8_t type;             // Partition type (0x0B, 0x0C for FAT32)
    uint8_t last_chs[3];      // Ignored
    uint32_t first_lba;       // Starting sector
    uint32_t sector_count;    // Number of sectors
} __attribute__((packed));

// MBR structure (simplified)
struct mbr {
    uint8_t bootstrap[446];   // Bootstrap code
    struct mbr_partition partitions[4]; // Partition table
    uint16_t signature;       // 0xAA55
} __attribute__((packed));

// Function to parse MBR and find FAT32 partition
int mbr_parse(block_dev_t *dev, uint32_t *fat32_lba, uint32_t *fat32_size);

#endif