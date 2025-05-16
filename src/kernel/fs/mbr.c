#include <kernel/fs/mbr.h>
#include <stdio.h>

// Parse MBR and find FAT32 partition
int mbr_parse(struct block_dev *dev, uint32_t *fat32_lba, uint32_t *fat32_size) {
    uint8_t buffer[512];
    struct mbr *mbr = (struct mbr *)buffer;

    // Read MBR (LBA 0)
    if (dev->read(0, buffer, 1) != 0) {
        serial_printf("Failed to read MBR\n");
        return -1;
    }

    // Verify MBR signature
    if (mbr->signature != 0xAA55) {
        serial_printf("Invalid MBR signature: 0x%04x\n", mbr->signature);
        return -1;
    }

    // Parse partition table
    for (int i = 0; i < 4; i++) {
        struct mbr_partition *part = &mbr->partitions[i];

        // Skip empty or invalid partitions
        if (part->type == 0x00) {
            continue;
        }

        // Check for FAT32 (0x0B or 0x0C)
        if (part->type == 0x0B || part->type == 0x0C) {
            *fat32_lba = part->first_lba;
            *fat32_size = part->sector_count;
            serial_printf("FAT32 partition found at LBA %u, size %u sectors\n", *fat32_lba, *fat32_size);
            return 0;
        }

        // Warn about extended partitions
        if (part->type == 0x05 || part->type == 0x0F) {
            serial_printf("Warning: Extended partition (type 0x%x) at index %u, skipping\n", part->type, i);
        }
    }

    serial_printf("No FAT32 partition found\n");
    return -1;
}