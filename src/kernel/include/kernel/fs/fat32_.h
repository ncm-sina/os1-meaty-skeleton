#ifndef FAT32_H
#define FAT32_H

#include <kernel/fs/fat32.h>
#include <kernel/fs/vfs.h>
#include <kernel/drivers/ide.h>
#include <stdint.h>

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



// #endif

// #ifndef FAT32_H
// #define FAT32_H

// #include <stdbool.h>
// #include <stddef.h>
// #include <stdint.h>

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

// // Directory entry (short file name)
// typedef struct {
//     char name[8];
//     char ext[3];
//     uint8_t attributes;
//     uint8_t reserved;
//     uint8_t create_time_tenth;
//     uint16_t create_time;
//     uint16_t create_date;
//     uint16_t access_date;
//     uint16_t first_cluster_high;
//     uint16_t modify_time;
//     uint16_t modify_date;
//     uint16_t first_cluster_low;
//     uint32_t file_size;
// }__attribute__((packed)) fat32_dir_entry_t;

// // Long file name entry
// typedef struct {
//     uint8_t sequence_number;
//     uint16_t name1[5];
//     uint8_t attributes; // Always 0x0F
//     uint8_t type;
//     uint8_t checksum;
//     uint16_t name2[6];
//     uint16_t first_cluster_low; // Always 0
//     uint16_t name3[2];
// }__attribute__((packed)) fat32_lfn_entry_t;

// FAT32 filesystem context
typedef struct {
    block_dev_t *dev;         // IDE device (set internally)
    uint32_t partition_lba;        // Start of FAT32 partition
    fat32_boot_sector_t bpb;  // Boot sector parameters
    uint32_t fat_start_lba;        // First sector of FAT
    uint32_t data_start_lba;       // First sector of data
}__attribute__((packed)) fat32_fs_t;


// bool is_valid_fat32_lfn_char(uint16_t unit);

// // Initialize FAT32 filesystem
// int fat32_init(fat32_fs_t *fs, uint32_t partition_lba);

// // List directory contents
// void fat32_list_dir(fat32_fs_t *fs, uint32_t cluster);

// // Read file contents
// int fat32_read_file(fat32_fs_t *fs, uint32_t cluster, uint32_t size, uint8_t *buffer);

int fat32_init(fat32_fs_t *fs, uint32_t partition_lba);
int fat32_mount(fat32_fs_t *fs);

#endif