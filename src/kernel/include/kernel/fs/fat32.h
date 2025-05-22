#ifndef FAT32_H
#define FAT32_H

#include <kernel/fs/vfs.h>
#include <stdint.h>
#include <kernel/fs/fat32.h>
#include <kernel/drivers/ide.h>
#include <stdlib.h>
#include <string.h>

// struct block_dev {
//     int (*read_sectors)(uint32_t lba, uint8_t *buffer, uint32_t count);
// };

// struct fat32_fs {
//     struct block_dev *dev;
//     uint32_t partition_lba;
//     struct {
//         uint16_t bytes_per_sector;
//         uint8_t sectors_per_cluster;
//         uint16_t reserved_sectors;
//         uint8_t num_fats;
//         uint32_t fat_size;
//         uint32_t root_cluster;
//         uint32_t total_sectors;
//     } bpb;
//     uint32_t fat_start_lba;
//     uint32_t data_start_lba;
// };

// FAT32 boot sector (simplified BPB)
typedef struct  {
    uint16_t bytes_per_sector;     // BPB_BytsPerSec
    uint8_t sectors_per_cluster;   // BPB_SecPerClus
    uint16_t reserved_sectors;     // BPB_RsvdSecCnt
    uint8_t num_fats;              // BPB_NumFATs
    uint32_t fat_size;             // BPB_FATSz32
    uint32_t root_cluster;         // BPB_RootClus
    uint32_t total_sectors;        // BPB_TotSec32
}__attribute__((packed)) fat32_boot_sector_t;

// FAT32 filesystem context
typedef struct {
    block_dev_t *dev;         // IDE device (set internally)
    uint32_t partition_lba;        // Start of FAT32 partition
    fat32_boot_sector_t bpb;  // Boot sector parameters
    uint32_t fat_start_lba;        // First sector of FAT
    uint32_t data_start_lba;       // First sector of data
}__attribute__((packed)) fat32_fs_t;

int fat32_init(fat32_fs_t *fs, uint32_t partition_lba);
int fat32_mount(fat32_fs_t *fs);

#endif