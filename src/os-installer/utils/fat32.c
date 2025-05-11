#include "fat32.h"
#include <string.h>
#include "memory.h"

static Disk* current_disk;
static BootSector bs;
static uint32_t fat_start;
static uint32_t data_start;
static uint8_t sector_buffer[512];

static uint32_t get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start + (fat_offset / bs.bytes_per_sector);
    uint32_t fat_entry_offset = fat_offset % bs.bytes_per_sector;

    if (ahci_read_sectors(current_disk, fat_sector, 1, sector_buffer) != 0) return 0;
    uint32_t next_cluster;
    memcpy(&next_cluster, sector_buffer + fat_entry_offset, 4);
    return next_cluster & 0x0FFFFFFF;
}

static uint32_t allocate_cluster(uint32_t prev_cluster) {
    uint32_t cluster = 2;
    uint32_t max_clusters = (bs.total_sectors - data_start) / bs.sectors_per_cluster;
    while (cluster < max_clusters) {
        if (get_next_cluster(cluster) == 0) {
            uint32_t fat_offset = cluster * 4;
            uint32_t fat_sector = fat_start + (fat_offset / bs.bytes_per_sector);
            uint32_t fat_entry_offset = fat_offset % bs.bytes_per_sector;
            uint32_t eoc = 0x0FFFFFFF;
            if (ahci_read_sectors(current_disk, fat_sector, 1, sector_buffer) != 0) return 0;
            memcpy(sector_buffer + fat_entry_offset, &eoc, 4);
            if (ahci_write_sectors(current_disk, fat_sector, 1, sector_buffer) != 0) return 0;

            if (prev_cluster != 0) {
                fat_offset = prev_cluster * 4;
                fat_sector = fat_start + (fat_offset / bs.bytes_per_sector);
                fat_entry_offset = fat_offset % bs.bytes_per_sector;
                if (ahci_read_sectors(current_disk, fat_sector, 1, sector_buffer) != 0) return 0;
                memcpy(sector_buffer + fat_entry_offset, &cluster, 4);
                if (ahci_write_sectors(current_disk, fat_sector, 1, sector_buffer) != 0) return 0;
            }
            return cluster;
        }
        cluster++;
    }
    return 0;
}

static uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start + (cluster - 2) * bs.sectors_per_cluster;
}

void name_to_fat32(const char* name, char* fat_name) {
    memset(fat_name, ' ', 11);
    const char* dot = strrchr(name, '.');
    if (dot) {
        int base_len = dot - name;
        int ext_len = strlen(dot + 1);
        if (base_len > 8) base_len = 8;
        if (ext_len > 3) ext_len = 3;
        memcpy(fat_name, name, base_len);
        memcpy(fat_name + 8, dot + 1, ext_len);
    } else {
        int len = strlen(name);
        if (len > 8) len = 8;
        memcpy(fat_name, name, len);
    }
}

static int fat32_list_dir_cluster(uint32_t cluster, DirEntry* entries, uint32_t* count) {
    uint32_t entry_count = 0;
    while (cluster != 0x0FFFFFFF && entry_count < *count) {
        uint32_t lba = cluster_to_lba(cluster);
        uint32_t sectors = bs.sectors_per_cluster;
        for (uint32_t s = 0; s < sectors && entry_count < *count; s++) {
            if (ahci_read_sectors(current_disk, lba + s, 1, sector_buffer) != 0) return -1;
            for (int i = 0; i < bs.bytes_per_sector / sizeof(DirEntry); i++) {
                DirEntry* entry = (DirEntry*)(sector_buffer + i * sizeof(DirEntry));
                if (entry->name[0] == 0x00) {
                    *count = entry_count;
                    return 0;
                }
                if (entry->name[0] != 0xE5 && entry_count < *count) {
                    memcpy(&entries[entry_count++], entry, sizeof(DirEntry));
                }
            }
        }
        cluster = get_next_cluster(cluster);
    }
    *count = entry_count;
    return 0;
}

static uint32_t find_dir(const char* path, char* name_out) {
    char path_copy[256];
    strncpy(path_copy, path, 256);
    char* token = path_copy;
    uint32_t cluster = bs.root_cluster;

    if (path[0] == '/') token++;

    while (*token) {
        char* next = strchr(token, '/');
        if (next) *next = '\0';

        uint32_t count = 128;
        DirEntry entries[128];
        if (fat32_list_dir_cluster(cluster, entries, &count) != 0) return 0;
        for (uint32_t i = 0; i < count; i++) {
            if (entries[i].attr & 0x10) { // Directory
                char entry_name[12];
                memcpy(entry_name, entries[i].name, 11);
                entry_name[11] = '\0';
                char fat_name[12];
                name_to_fat32(token, fat_name);
                if (strncmp(entry_name, fat_name, 11) == 0) {
                    cluster = (entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
                    break;
                }
            }
        }
        if (!next) {
            name_to_fat32(token, name_out);
            return cluster;
        }
        token = next + 1;
    }
    name_to_fat32("", name_out);
    return cluster;
}


int fat32_format(Disk* disk, uint32_t partition_lba, uint32_t size) {
    BootSector bs_local;
    memset(&bs_local, 0, sizeof(BootSector));
    // printf(" 1 ");
    bs_local.jump[0] = 0xEB;
    bs_local.jump[1] = 0x58;
    bs_local.jump[2] = 0x90;
    strncpy(bs_local.oem_name, "MSWIN4.1", 8);
    // printf(" 2 ");
    bs_local.bytes_per_sector = 512;
    bs_local.sectors_per_cluster = 8;
    bs_local.reserved_sectors = 32;
    bs_local.num_fats = 2;
    bs_local.media_type = 0xF8;
    bs_local.total_sectors = size;
    bs_local.root_cluster = 2;
    bs_local.fs_info = 1;
    bs_local.backup_boot = 6;
    bs_local.drive_number = 0x80;
    bs_local.boot_signature = 0x29;
    bs_local.volume_id = 0x12345678;
    strncpy(bs_local.volume_label, "OS_PART    ", 11);
    strncpy(bs_local.fs_type, "FAT32   ", 8);
    // printf(" 3 ");
    bs_local.signature = 0xAA55;

    uint32_t total_clusters = (size - bs_local.reserved_sectors) / bs_local.sectors_per_cluster;
    bs_local.sectors_per_fat = (total_clusters * 4 + bs_local.bytes_per_sector - 1) / bs_local.bytes_per_sector;

    // printf(" 4 ");
    current_disk = disk;
    if (ahci_write_sectors(disk, partition_lba, 1, &bs_local) != 0) return -1;

    uint32_t fat_size = bs_local.sectors_per_fat;
    printf(" 5 fat_size:%08x %08x ", fat_size, fat_size * bs_local.bytes_per_sector);
    uint8_t* fat = (uint8_t*) kmalloc(fat_size * bs_local.bytes_per_sector);
    printf(" fat:%08x *fat:%08x ", fat,  *fat);
    memset(fat, 0, fat_size * bs_local.bytes_per_sector);
    printf(" 6 ");
    uint32_t* fat_entries = (uint32_t*)fat;
    fat_entries[0] = 0x0FFFFFF8;
    fat_entries[1] = 0x0FFFFFFF;
    fat_entries[2] = 0x0FFFFFFF;
    printf(" 7 ");

    for (int i = 0; i < bs_local.num_fats; i++) {
        if (ahci_write_sectors(disk, partition_lba + bs_local.reserved_sectors + i * bs_local.sectors_per_fat, fat_size, fat) != 0) {
            printf(" 5 fat_size:%08x fat:%08x *fat:%08x ", fat_size, fat,  *fat);
            kfree(fat);
            while(1);
            return -1;
        }
    }
    printf(" 7 ");

    kfree(fat);

    uint32_t root_lba = partition_lba + bs_local.reserved_sectors + bs_local.num_fats * bs_local.sectors_per_fat;
    printf(" 8 ");
    uint8_t* root_dir = kmalloc(bs_local.sectors_per_cluster * bs_local.bytes_per_sector);
    printf(" 9 ");
    memset(root_dir, 0, bs_local.sectors_per_cluster * bs_local.bytes_per_sector);
    printf(" 10 ");
    if (ahci_write_sectors(disk, root_lba, bs_local.sectors_per_cluster, root_dir) != 0) {
        kfree(root_dir);
        return -1;
    }
    printf(" 11 ");
    kfree(root_dir);

    memcpy(&bs, &bs_local, sizeof(BootSector));
    fat_start = partition_lba + bs.reserved_sectors;
    data_start = fat_start + (bs.num_fats * bs.sectors_per_fat);
    return 0;
}

int fat32_mount(Disk* disk, uint32_t partition_lba) {
    current_disk = disk;
    if (ahci_read_sectors(disk, partition_lba, 1, &bs) != 0) return -1;
    if (strncmp(bs.fs_type, "FAT32   ", 8) != 0) return -1;
    fat_start = partition_lba + bs.reserved_sectors;
    data_start = fat_start + (bs.num_fats * bs.sectors_per_fat);
    return 0;
}

int fat32_unmount(void) {
    current_disk = NULL;
    return 0;
}

int fat32_create_file(const char* path, const uint8_t* data, uint32_t size) {
    char parent[256], name[12];
    uint32_t parent_cluster = find_dir(path, name);
    if (parent_cluster == 0) return -1;

    uint32_t cluster = allocate_cluster(0);
    if (cluster == 0) return -1;

    DirEntry entry;
    memset(&entry, 0, sizeof(DirEntry));
    memcpy(entry.name, name, 11);
    entry.attr = 0x20;
    entry.first_cluster_low = cluster & 0xFFFF;
    entry.first_cluster_high = (cluster >> 16) & 0xFFFF;
    entry.file_size = size;

    uint32_t dir_lba = cluster_to_lba(parent_cluster);
    uint32_t sectors = bs.sectors_per_cluster;
    for (uint32_t s = 0; s < sectors; s++) {
        if (ahci_read_sectors(current_disk, dir_lba + s, 1, sector_buffer) != 0) return -1;
        for (int i = 0; i < bs.bytes_per_sector / sizeof(DirEntry); i++) {
            DirEntry* slot = (DirEntry*)(sector_buffer + i * sizeof(DirEntry));
            if (slot->name[0] == 0x00 || slot->name[0] == 0xE5) {
                memcpy(slot, &entry, sizeof(DirEntry));
                if (ahci_write_sectors(current_disk, dir_lba + s, 1, sector_buffer) != 0) return -1;
                // goto write_data;
                // write_data:
                uint32_t current_cluster = cluster;
                uint32_t offset = 0;
                while (offset < size) {
                    uint32_t lba = cluster_to_lba(current_cluster);
                    uint32_t chunk_size = (size - offset > bs.sectors_per_cluster * bs.bytes_per_sector) ?
                                          bs.sectors_per_cluster * bs.bytes_per_sector : size - offset;
                    uint32_t chunk_sectors = (chunk_size + bs.bytes_per_sector - 1) / bs.bytes_per_sector;
                    memcpy(sector_buffer, data + offset, chunk_size);
                    if (ahci_write_sectors(current_disk, lba, chunk_sectors, sector_buffer) != 0) return -1;
                    offset += chunk_size;
                    if (offset < size) {
                        current_cluster = allocate_cluster(current_cluster);
                        if (current_cluster == 0) return -1;
                    }
                }
                return 0;            
            }
        }
    }
    return -1;

}

int fat32_create_dir(const char* path) {
    char parent[256], name[12];
    uint32_t parent_cluster = find_dir(path, name);
    if (parent_cluster == 0) return -1;

    uint32_t cluster = allocate_cluster(0);
    if (cluster == 0) return -1;

    DirEntry entry;
    memset(&entry, 0, sizeof(DirEntry));
    memcpy(entry.name, name, 11);
    entry.attr = 0x10;
    entry.first_cluster_low = cluster & 0xFFFF;
    entry.first_cluster_high = (cluster >> 16) & 0xFFFF;

    uint32_t dir_lba = cluster_to_lba(parent_cluster);
    uint32_t sectors = bs.sectors_per_cluster;
    for (uint32_t s = 0; s < sectors; s++) {
        if (ahci_read_sectors(current_disk, dir_lba + s, 1, sector_buffer) != 0) return -1;
        for (int i = 0; i < bs.bytes_per_sector / sizeof(DirEntry); i++) {
            DirEntry* slot = (DirEntry*)(sector_buffer + i * sizeof(DirEntry));
            if (slot->name[0] == 0x00 || slot->name[0] == 0xE5) {
                memcpy(slot, &entry, sizeof(DirEntry));
                if (ahci_write_sectors(current_disk, dir_lba + s, 1, sector_buffer) != 0) return -1;
                // goto init_dir;
                // return init_dir();
                // int init_dir();
                uint32_t new_lba = cluster_to_lba(cluster);
                memset(sector_buffer, 0, bs.bytes_per_sector);
                DirEntry* dot = (DirEntry*)sector_buffer;
                memcpy(dot->name, ".          ", 11);
                dot->attr = 0x10;
                dot->first_cluster_low = cluster & 0xFFFF;
                dot->first_cluster_high = (cluster >> 16) & 0xFFFF;
                DirEntry* dotdot = (DirEntry*)(sector_buffer + sizeof(DirEntry));
                memcpy(dotdot->name, "..         ", 11);
                dotdot->attr = 0x10;
                dotdot->first_cluster_low = parent_cluster & 0xFFFF;
                dotdot->first_cluster_high = (parent_cluster >> 16) & 0xFFFF;
                if (ahci_write_sectors(current_disk, new_lba, 1, sector_buffer) != 0) return -1;
                return 0;                
            }
        }
    }
    return -1;


}


int fat32_list_dir(const char* path, DirEntry* entries, uint32_t* count) {
    char name[12];
    uint32_t cluster = find_dir(path, name);
    if (cluster == 0) return -1;
    return fat32_list_dir_cluster(cluster, entries, count);
}

int fat32_read_file(const char* path, uint8_t* buffer, uint32_t* size) {
    char parent[256], name[12];
    uint32_t parent_cluster = find_dir(path, name);
    if (parent_cluster == 0) return -1;

    uint32_t count = 128;
    DirEntry entries[128];
    if (fat32_list_dir_cluster(parent_cluster, entries, &count) != 0) return -1;
    for (uint32_t i = 0; i < count; i++) {
        if (!(entries[i].attr & 0x10) && memcmp(entries[i].name, name, 11) == 0) {
            uint32_t cluster = (entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            uint32_t file_size = entries[i].file_size;
            uint32_t offset = 0;
            while (cluster != 0x0FFFFFFF && offset < file_size) {
                uint32_t lba = cluster_to_lba(cluster);
                uint32_t chunk_size = (file_size - offset > bs.sectors_per_cluster * bs.bytes_per_sector) ?
                                      bs.sectors_per_cluster * bs.bytes_per_sector : file_size - offset;
                uint32_t chunk_sectors = (chunk_size + bs.bytes_per_sector - 1) / bs.bytes_per_sector;
                if (ahci_read_sectors(current_disk, lba, chunk_sectors, buffer + offset) != 0) return -1;
                offset += chunk_size;
                cluster = get_next_cluster(cluster);
            }
            *size = file_size;
            return 0;
        }
    }
    return -1;
}