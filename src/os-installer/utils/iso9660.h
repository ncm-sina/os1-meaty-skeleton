#ifndef ISO9660_H
#define ISO9660_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ahci.h"
#include "fat32.h"

typedef struct {
    uint8_t type;
    char identifier[5];
    uint8_t version;
    uint8_t reserved[1];
    char system_id[32];
    char volume_id[32];
    uint8_t reserved2[8];
    uint32_t volume_space_size;
    uint32_t volume_space_size_be;
    uint8_t reserved3[32];
    uint16_t volume_set_size;
    uint16_t volume_set_size_be;
    uint16_t volume_sequence_number;
    uint16_t volume_sequence_number_be;
    uint16_t logical_block_size;
    uint16_t logical_block_size_be;
    uint32_t path_table_size;
    uint32_t path_table_size_be;
    uint32_t l_path_table;
    uint32_t l_path_table_opt;
    uint32_t m_path_table;
    uint32_t m_path_table_opt;
    uint8_t root_directory[34];
    char volume_set_id[128];
    char publisher_id[128];
    char data_preparer_id[128];
    char application_id[128];
    char copyright_file_id[37];
    char abstract_file_id[37];
    char bibliographic_file_id[37];
    uint8_t creation_date[17];
    uint8_t modification_date[17];
    uint8_t expiration_date[17];
    uint8_t effective_date[17];
    uint8_t file_structure_version;
    uint8_t reserved4;
    uint8_t application_data[512];
    uint8_t reserved5[653];
} __attribute__((packed)) VolumeDescriptor;

typedef struct {
    uint8_t length;
    uint8_t ext_attr_length;
    uint32_t extent_lba;
    uint32_t extent_lba_be;
    uint32_t size;
    uint32_t size_be;
    uint8_t date[7];
    uint8_t flags;
    uint8_t file_unit_size;
    uint8_t interleave;
    uint16_t volume_seq;
    uint16_t volume_seq_be;
    uint8_t name_len;
    char name[1];
} __attribute__((packed)) DirectoryRecord;

typedef struct {
    char* name;
    DirectoryRecord record;
} ISO9660_DirEntry;

// typedef struct {
//     uint8_t type;
//     char identifier[5];
//     uint8_t version;
//     uint8_t reserved[1];
//     char system_id[32];
//     char volume_id[32];
//     uint8_t reserved2[8];
//     uint32_t volume_space_size;
//     uint32_t volume_space_size_be;
//     uint8_t reserved3[32];
//     uint16_t volume_set_size;
//     uint16_t volume_set_size_be;
//     uint16_t volume_sequence_number;
//     uint16_t volume_sequence_number_be;
//     uint16_t logical_block_size;
//     uint16_t logical_block_size_be;
//     uint32_t path_table_size;
//     uint32_t path_table_size_be;
//     uint32_t l_path_table;
//     uint32_t l_path_table_opt;
//     uint32_t m_path_table;
//     uint32_t m_path_table_opt;
//     uint8_t root_directory[34];
//     char volume_set_id[128];
//     char publisher_id[128];
//     char data_preparer_id[128];
//     char application_id[128];
//     char copyright_file_id[37];
//     char abstract_file_id[37];
//     char bibliographic_file_id[37];
//     uint8_t creation_date[17];
//     uint8_t modification_date[17];
//     uint8_t expiration_date[17];
//     uint8_t effective_date[17];
//     uint8_t file_structure_version;
//     uint8_t reserved4;
//     uint8_t application_data[512];
//     uint8_t reserved5[653];
// } __attribute__((packed)) VolumeDescriptor;

// typedef struct {
//     uint8_t length;
//     uint8_t ext_attr_length;
//     uint32_t extent_lba;
//     uint32_t extent_lba_be;
//     uint32_t size;
//     uint32_t size_be;
//     uint8_t date[7];
//     uint8_t flags;
//     uint8_t file_unit_size;
//     uint8_t interleave;
//     uint16_t volume_seq;
//     uint16_t volume_seq_be;
//     uint8_t name_len;
//     char name[1];
// } __attribute__((packed)) DirectoryRecord;

int iso9660_mount(Disk* disk);
int iso9660_unmount(void);
int iso9660_read_file(const char* path, uint8_t* buffer, uint32_t* size);
int iso9660_list_dir(const char* path, DirEntry* entries, uint32_t* count);

#endif // ISO9660_H