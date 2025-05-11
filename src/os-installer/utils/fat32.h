#ifndef FAT32_H
#define FAT32_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ahci.h"

typedef struct {
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_small;
    uint8_t media_type;
    uint16_t sectors_per_fat_small;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors;
    uint32_t sectors_per_fat;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[420];
    uint16_t signature;
} __attribute__((packed)) BootSector;

typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) DirEntry;


// typedef struct {
//     uint8_t jump[3];
//     char oem_name[8];
//     uint16_t bytes_per_sector;
//     uint8_t sectors_per_cluster;
//     uint16_t reserved_sectors;
//     uint8_t num_fats;
//     uint16_t root_entries;
//     uint16_t total_sectors_small;
//     uint8_t media_type;
//     uint16_t sectors_per_fat_small;
//     uint16_t sectors_per_track;
//     uint16_t num_heads;
//     uint32_t hidden_sectors;
//     uint32_t total_sectors;
//     uint32_t sectors_per_fat;
//     uint16_t ext_flags;
//     uint16_t fs_version;
//     uint32_t root_cluster;
//     uint16_t fs_info;
//     uint16_t backup_boot;
//     uint8_t reserved[12];
//     uint8_t drive_number;
//     uint8_t reserved1;
//     uint8_t boot_signature;
//     uint32_t volume_id;
//     char volume_label[11];
//     char fs_type[8];
//     uint8_t boot_code[420];
//     uint16_t signature;
// } __attribute__((packed)) BootSector;

// typedef struct {
//     char name[11];
//     uint8_t attr;
//     uint8_t reserved;
//     uint8_t create_time_tenth;
//     uint16_t create_time;
//     uint16_t create_date;
//     uint16_t last_access_date;
//     uint16_t first_cluster_high;
//     uint16_t write_time;
//     uint16_t write_date;
//     uint16_t first_cluster_low;
//     uint32_t file_size;
//     DirectoryRecord record;
// } __attribute__((packed)) DirEntry;

int fat32_format(Disk* disk, uint32_t partition_lba, uint32_t size);
int fat32_mount(Disk* disk, uint32_t partition_lba);
int fat32_unmount(void);
void name_to_fat32(const char* name, char* fat_name);
int fat32_create_file(const char* path, const uint8_t* data, uint32_t size);
int fat32_create_dir(const char* path);
int fat32_list_dir(const char* path, DirEntry* entries, uint32_t* count);
int fat32_read_file(const char* path, uint8_t* buffer, uint32_t* size);

#endif // FAT32_H