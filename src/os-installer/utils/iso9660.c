#include "iso9660.h"
#include "fat32.h"
#include <string.h>
#include "memory.h"

static Disk* cdrom_disk;
static uint32_t root_lba;
static uint16_t block_size;
static uint8_t sector_buffer[2048];

static DirectoryRecord* find_file(const char* path) {
    char path_copy[256];
    strncpy(path_copy, path, 256);
    char* token = path_copy;
    uint32_t current_lba = root_lba;
    uint8_t* dir_buffer = NULL;
    uint32_t dir_size = 0;

    if (path[0] == '/') token++;

    while (*token) {
        char* next = strchr(token, '/');
        if (next) *next = '\0';

        if (dir_buffer) kfree(dir_buffer);
        DirectoryRecord* dir = (DirectoryRecord*)sector_buffer;
        if (ahci_read_sectors(cdrom_disk, current_lba, 1, sector_buffer) != 0) return NULL;
        dir_size = dir->size;
        dir_buffer = kmalloc(dir_size);
        uint32_t sectors = (dir_size + block_size - 1) / block_size;
        if (ahci_read_sectors(cdrom_disk, current_lba, sectors, dir_buffer) != 0) {
            kfree(dir_buffer);
            return NULL;
        }

        for (uint32_t offset = 0; offset < dir_size;) {
            DirectoryRecord* record = (DirectoryRecord*)(dir_buffer + offset);
            if (record->length == 0) break;
            if (record->name_len > 0 && record->name[0] != 0 && record->name[0] != 1) {
                char name[256];
                strncpy(name, record->name, record->name_len);
                name[record->name_len] = '\0';
                if (strcmp(name, token) == 0) {
                    current_lba = record->extent_lba;
                    if (!next) {
                        memcpy(sector_buffer, record, record->length);
                        kfree(dir_buffer);
                        return (DirectoryRecord*)sector_buffer;
                    }
                    break;
                }
            }
            offset += record->length;
        }
        if (!next) break;
        token = next + 1;
    }
    if (dir_buffer) kfree(dir_buffer);
    return NULL;
}


int iso9660_mount(Disk* disk) {
    cdrom_disk = disk;
    VolumeDescriptor vd;
    // printf("Mounting CDROM, lba=%u\n", lba);
    for (uint32_t lba = 16; lba < 32; lba++) {
        if (ahci_read_sectors2(disk, lba, 1, &vd, AHCI_CMD_READ) != 0) {
            printf("Failed to read LBA %u, TFD=0x%08x, SERR=0x%08x\n", lba, disk->port->tfd, disk->port->serr);
            print_hba_port(disk->port);
            continue;
        }
        print_volume_descriptor(&vd);
        if (strncmp(vd.identifier, "CD001", 5) == 0 && vd.type == 1) {
            print_directory_record((DirectoryRecord*)vd.root_directory);
            root_lba = ((DirectoryRecord*)vd.root_directory)->extent_lba;
            block_size = vd.logical_block_size;
            printf("ISO9660 mounted: root_lba=%u, block_size=%u\n", root_lba, block_size);
            return 0;
        }
    }
    printf("No valid ISO9660 volume descriptor found\n");
    return -1;
}
// int iso9660_mount(Disk* disk) {
//     cdrom_disk = disk;
//     VolumeDescriptor vd;
//     for (uint32_t lba = 16; lba < 32; lba++) {
//         printf("lba=%02x ", disk, lba);
//         if (ahci_read_sectors(disk, lba, 1, &vd) != 0){
//             printf(" xx ");
//             continue;
//         }
//         printf(" ------------------- ");
//         print_volume_descriptor(&vd);        
//         if (strncmp(vd.identifier, "CD001", 5) == 0 && vd.type == 1) {
//             printf(" yy ");
//             root_lba = ((DirectoryRecord*)vd.root_directory)->extent_lba;
//             block_size = vd.logical_block_size;
//             return 0;
//         }
//     }
//     return -1;
// }

int iso9660_unmount(void) {
    cdrom_disk = NULL;
    return 0;
}

int iso9660_read_file(const char* path, uint8_t* buffer, uint32_t* size) {
    DirectoryRecord* record = find_file(path);
    if (!record || (record->flags & 0x02)) return -1;

    uint32_t lba = record->extent_lba;
    uint32_t file_size = record->size;
    uint32_t sectors = (file_size + block_size - 1) / block_size;
    if (ahci_read_sectors(cdrom_disk, lba, sectors, buffer) != 0) return -1;
    *size = file_size;
    return 0;
}

int iso9660_list_dir(const char* path, DirEntry* entries, uint32_t* count) {
    DirectoryRecord* dir = find_file(path);
    if (!dir || !(dir->flags & 0x02)) return -1;

    uint32_t lba = dir->extent_lba;
    uint32_t dir_size = dir->size;
    uint8_t* dir_buffer = kmalloc(dir_size);
    uint32_t sectors = (dir_size + block_size - 1) / block_size;
    if (ahci_read_sectors(cdrom_disk, lba, sectors, dir_buffer) != 0) {
        kfree(dir_buffer);
        return -1;
    }

    uint32_t entry_count = 0;
    for (uint32_t offset = 0; offset < dir_size && entry_count < *count;) {
        DirectoryRecord* record = (DirectoryRecord*)(dir_buffer + offset);
        if (record->length == 0) break;
        if (record->name_len > 0 && record->name[0] != 0 && record->name[0] != 1) {
            DirEntry entry;
            memset(&entry, 0, sizeof(DirEntry));
            char name[12];
            strncpy(name, record->name, record->name_len < 11 ? record->name_len : 11);
            name_to_fat32(name, entry.name);
            entry.attr = (record->flags & 0x02) ? 0x10 : 0x20;
            entry.file_size = record->size;
            entries[entry_count++] = entry;
        }
        offset += record->length;
    }
    kfree(dir_buffer);
    *count = entry_count;
    return 0;
}