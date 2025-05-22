#include <kernel/fs/fat32.h>
#include <kernel/drivers/ide.h>
#include <stdlib.h>
#include <string.h>

// FAT32 directory entry
struct fat32_dir_entry {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_high;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed));

// LFN entry
struct fat32_lfn_entry {
    uint8_t sequence_number;
    uint16_t name1[5];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t first_cluster_low;
    uint16_t name3[2];
} __attribute__((packed));

// File operations
static int fat32_open(struct vfs_node *node, int flags, mode_t mode);
static int fat32_read(struct vfs_node *node, void *buf, size_t count, off_t offset);
static int fat32_write(struct vfs_node *node, const void *buf, size_t count, off_t offset);
static int fat32_readdir(struct vfs_node *node, struct dirent *entry, size_t *index);
static int fat32_close(struct vfs_node *node);
static int fat32_unlink(struct vfs_node *node);
static int fat32_stat(struct vfs_node *node, struct stat *statbuf);
static int fat32_access(struct vfs_node *node, int mode);

static struct file_operations fat32_ops = {
    .open = fat32_open,
    .read = fat32_read,
    .write = fat32_write,
    .readdir = fat32_readdir,
    .close = fat32_close,
    .unlink = fat32_unlink,
    .stat = fat32_stat,
    .access = fat32_access
};


static struct vfs_node *root;
// ... (Previous fat32_read_fat_entry, fat32_next_cluster, fat32_cluster_to_lba, fat32_read_cluster, compute_sfn_checksum unchanged)

int fat32_init(fat32_fs_t *fs, uint32_t partition_lba) {
    struct block_dev *dev = ide_get_block_dev();
    if (!dev) return -EIO;
    fs->dev = dev;
    fs->partition_lba = partition_lba;

    uint8_t buffer[512];
    if (ide_read_sectors(partition_lba, buffer, 1) != 0) return -EIO;

    fs->bpb.bytes_per_sector = *(uint16_t *)(buffer + 0x0B);
    fs->bpb.sectors_per_cluster = buffer[0x0D];
    fs->bpb.reserved_sectors = *(uint16_t *)(buffer + 0x0E);
    fs->bpb.num_fats = buffer[0x10];
    fs->bpb.fat_size = *(uint32_t *)(buffer + 0x24);
    fs->bpb.root_cluster = *(uint32_t *)(buffer + 0x2C);
    fs->bpb.total_sectors = *(uint32_t *)(buffer + 0x20);

    if (*(uint16_t *)(buffer + 0x1FE) != 0xAA55) return -EIO;
    if (fs->bpb.bytes_per_sector != 512) return -EINVAL;

    fs->fat_start_lba = partition_lba + fs->bpb.reserved_sectors;
    fs->data_start_lba = fs->fat_start_lba + (fs->bpb.num_fats * fs->bpb.fat_size);
    root->private_data = fs; // Set FS context
    return 0;
}

int fat32_mount(fat32_fs_t *fs) {
    return vfs_mount("/", &fat32_ops);
}

static int fat32_open(struct vfs_node *node, int flags, mode_t mode) {
    if (flags & O_WRONLY) return -EACCES; // Read-only for now
    node->private_data = root->private_data;
    return 0;
}

// Convert cluster to LBA
static uint32_t fat32_cluster_to_lba(fat32_fs_t *fs, uint32_t cluster) {
    return fs->data_start_lba + (cluster - 2) * fs->bpb.sectors_per_cluster;
}

// Read a 32-bit FAT entry
static uint32_t fat32_read_fat_entry(fat32_fs_t *fs, uint32_t cluster) {
    block_dev_t *dev = ide_get_block_dev();
    if (!dev) {
        serial_printf("FAT32: No IDE device\n");
        return 0xFFFFFFFF;
    }

    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_lba + (fat_offset / fs->bpb.bytes_per_sector);
    uint32_t entry_offset = fat_offset % fs->bpb.bytes_per_sector;

    uint8_t buffer[512];
    if (ide_read_sectors(fat_sector, buffer, 1) != 0) {
        serial_printf("FAT32: Failed to read FAT sector %u\n", fat_sector);
        return 0xFFFFFFFF;
    }

    uint32_t entry = *(uint32_t *)(buffer + entry_offset);
    return entry & 0x0FFFFFFF; // Mask high 4 bits
}


// Get next cluster in chain
static uint32_t fat32_next_cluster(fat32_fs_t *fs, uint32_t cluster) {
    uint32_t next = fat32_read_fat_entry(fs, cluster);
    if (next >= 0x0FFFFFF8) return 0xFFFFFFFF; // End of chain
    return next;
}


// Read cluster data
static int fat32_read_cluster(fat32_fs_t *fs, uint32_t cluster, uint8_t *buffer) {
    block_dev_t *dev = ide_get_block_dev();
    if (!dev) {
        serial_printf("FAT32: No IDE device\n");
        return -1;
    }

    uint32_t lba = fat32_cluster_to_lba(fs, cluster);
    serial_printf("Reading LBA %u for cluster %u\n", lba, cluster);
    int status = ide_read_sectors(lba, buffer, fs->bpb.sectors_per_cluster);
    if (status != 0) {
        serial_printf("FAT32: Failed to read cluster %u, status %d\n", cluster, status);
    }
    return status;
}

static int fat32_read(struct vfs_node *node, void *buf, size_t count, off_t offset) {
    fat32_fs_t *fs = node->private_data;
    uint32_t cluster = node->cluster;
    uint32_t cluster_size = fs->bpb.sectors_per_cluster * fs->bpb.bytes_per_sector;
    uint8_t cluster_buffer[4096];
    size_t bytes_read = 0;

    while (offset >= cluster_size && cluster != 0xFFFFFFFF) {
        offset -= cluster_size;
        cluster = fat32_next_cluster(fs, cluster);
    }

    while (cluster != 0xFFFFFFFF && bytes_read < count) {
        if (fat32_read_cluster(fs, cluster, cluster_buffer) != 0) return -EIO;
        size_t to_copy = (count - bytes_read < cluster_size - offset) ? (count - bytes_read) : (cluster_size - offset);
        memcpy(buf + bytes_read, cluster_buffer + offset, to_copy);
        bytes_read += to_copy;
        offset = 0;
        cluster = fat32_next_cluster(fs, cluster);
    }

    return bytes_read;
}

// Compute SFN checksum for LFN validation
static uint8_t compute_sfn_checksum(const uint8_t *sfn) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + sfn[i];
    }
    return sum;
}

bool is_valid_fat32_ascii_char(char c) {
    if (c <= 0x1F || c == 0x7F) {
        return false;
    }
    switch (c) {
        case '"': case '*': case '/': case ':': case '<': case '>': case '?': case '\\': case '|':
            return false;
    }
    return true;
}

bool is_valid_fat32_lfn_char(uint16_t unit) {
    if (unit <= 0x1F || unit == 0x7F) return false;
    if (unit == '"' || unit == '*' || unit == '/' || unit == ':' || unit == '<' || unit == '>' ||
        unit == '?' || unit == '\\' || unit == '|') return false;
    if (unit >= 0xD800 && unit <= 0xDFFF) return false;
    if ((unit >= 0xFDD0 && unit <= 0xFDEF) || unit == 0xFFFE || unit == 0xFFFF) return false;
    if (unit >= 0xE000 && unit <= 0xF8FF) return false;
    if (unit == 0x200B || unit == 0x200C || unit == 0x200D || unit == 0xFEFF) return false;
    return true;
}


static int fat32_write(struct vfs_node *node, const void *buf, size_t count, off_t offset) {
    return -ENOSYS; // TODO: Implement
}

static int fat32_readdir(struct vfs_node *node, struct dirent *entry, size_t *index) {
    fat32_fs_t *fs = node->private_data;
    uint32_t cluster = node->cluster;
    uint8_t buffer[512];
    char lfn_buffer[256];
    int lfn_pos = 0;
    uint8_t lfn_checksum = 0;
    int lfn_sequence = 0;
    size_t current_index = 0;

    while (cluster != 0xFFFFFFFF) {
        uint32_t lba = fat32_cluster_to_lba(fs, cluster);
        for (uint32_t i = 0; i < fs->bpb.sectors_per_cluster; i++) {
            if (ide_read_sectors(lba + i, buffer, 1) != 0) return -EIO;
            for (uint32_t j = 0; j < 512; j += 32) {
                if (buffer[j] == 0x00) return 0;
                if (buffer[j] == 0xE5) continue;

                struct fat32_dir_entry *dir_entry = (struct fat32_dir_entry *)(buffer + j);
                if (dir_entry->attributes == 0x0F) {
                    struct fat32_lfn_entry *lfn = (struct fat32_lfn_entry *)(buffer + j);
                    int seq = lfn->sequence_number & 0x1F;
                    if (lfn->sequence_number & 0x40) {
                        lfn_pos = 0;
                        lfn_sequence = seq;
                        lfn_checksum = lfn->checksum;
                    }
                    if (seq != lfn_sequence && lfn_pos > 0) {
                        lfn_pos = 0;
                        lfn_sequence = 0;
                        continue;
                    }
                    for (int k = 0; k < 5; k++) if (lfn->name1[k] <= 0xFF && lfn_pos < 255) lfn_buffer[lfn_pos++] = (char)lfn->name1[k];
                    for (int k = 0; k < 6; k++) if (lfn->name2[k] <= 0xFF && lfn_pos < 255) lfn_buffer[lfn_pos++] = (char)lfn->name2[k];
                    for (int k = 0; k < 2; k++) if (lfn->name3[k] <= 0xFF && lfn_pos < 255) lfn_buffer[lfn_pos++] = (char)lfn->name3[k];
                    lfn_buffer[lfn_pos] = '\0';
                    lfn_sequence--;
                    continue;
                }

                if (current_index < *index) {
                    current_index++;
                    lfn_pos = 0;
                    lfn_sequence = 0;
                    continue;
                }

                char name[13];
                int k = 0;
                for (int m = 0; m < 8 && dir_entry->name[m] != ' '; m++) name[k++] = dir_entry->name[m];
                if (dir_entry->ext[0] != ' ') {
                    name[k++] = '.';
                    for (int m = 0; m < 3 && dir_entry->ext[m] != ' '; m++) name[k++] = dir_entry->ext[m];
                }
                name[k] = '\0';

                uint8_t sfn_raw[11];
                memcpy(sfn_raw, dir_entry->name, 8);
                memcpy(sfn_raw + 8, dir_entry->ext, 3);
                uint8_t computed_checksum = compute_sfn_checksum(sfn_raw);

                const char *final_name = (lfn_pos > 0 && lfn_sequence == 0 && computed_checksum == lfn_checksum) ? lfn_buffer : name;

                if (dir_entry->attributes & 0x08) {
                    lfn_pos = 0;
                    lfn_sequence = 0;
                    continue;
                }

                entry->d_ino = ((uint32_t)dir_entry->first_cluster_high << 16) | dir_entry->first_cluster_low;
                strncpy(entry->d_name, final_name, 255);
                entry->d_name[255] = '\0';
                entry->d_type = (dir_entry->attributes & 0x10) ? DT_DIR : DT_REG;
                *index += 1;
                return 1;
            }
        }
        cluster = fat32_next_cluster(fs, cluster);
    }

    return 0;
}

static int fat32_close(struct vfs_node *node) {
    return 0;
}

static int fat32_unlink(struct vfs_node *node) {
    return -ENOSYS; // TODO: Implement
}

static int fat32_stat(struct vfs_node *node, struct stat *statbuf) {
    return -ENOSYS; // TODO: Implement
}

static int fat32_access(struct vfs_node *node, int mode) {
    return 0; // FAT32: assume all access OK
}

// #include <stdarg.h>
// #include <kernel/drivers/serial.h>
// #include <kernel/drivers/ide.h>
// #include <kernel/fs/fat32.h>
// #include <kernel/fs/unicode.h>

// #define LFN_MAX_LEN 255
// #define MAX_LFN_ENTRIES 20 // Max LFN entries (255 chars / 13 chars per entry)

// // Structure to store LFN parts temporarily
// typedef struct {
//     uint16_t chars[13]; // 13 characters per LFN entry
//     int seq;            // Sequence number
// } lfn_part_t;




// // Initialize FAT32 filesystem
// int fat32_init(fat32_fs_t *fs, uint32_t partition_lba) {
//     block_dev_t *dev = ide_get_block_dev();
//     if (!dev) {
//         serial_printf("FAT32: No IDE device\n");
//         return -1;
//     }
//     fs->dev = dev;
//     fs->partition_lba = partition_lba;

//     uint8_t buffer[512];
//     if (ide_read_sectors(partition_lba, buffer, 1) != 0) {
//         serial_printf("FAT32: Failed to read boot sector at LBA %u\n", partition_lba);
//         return -1;
//     }

//     fs->bpb.bytes_per_sector = *(uint16_t *)(buffer + 0x0B);
//     fs->bpb.sectors_per_cluster = buffer[0x0D];
//     fs->bpb.reserved_sectors = *(uint16_t *)(buffer + 0x0E);
//     fs->bpb.num_fats = buffer[0x10];
//     fs->bpb.fat_size = *(uint32_t *)(buffer + 0x24);
//     fs->bpb.root_cluster = *(uint32_t *)(buffer + 0x2C);
//     fs->bpb.total_sectors = *(uint32_t *)(buffer + 0x20);

//     if (*(uint16_t *)(buffer + 0x1FE) != 0xAA55) {
//         serial_printf("FAT32: Invalid boot sector signature\n");
//         return -1;
//     }
//     if (fs->bpb.bytes_per_sector != 512) {
//         serial_printf("FAT32: Unsupported bytes per sector %u\n", fs->bpb.bytes_per_sector);
//         return -1;
//     }

//     fs->fat_start_lba = partition_lba + fs->bpb.reserved_sectors;
//     fs->data_start_lba = fs->fat_start_lba + (fs->bpb.num_fats * fs->bpb.fat_size);

//     serial_printf("FAT32: Initialized. Root cluster %u, FAT start LBA %u, Data start LBA %u\n",
//                   fs->bpb.root_cluster, fs->fat_start_lba, fs->data_start_lba);
//     return 0;
// }

// // Process a single LFN entry into a temporary part
// static bool process_lfn_entry(fat32_lfn_entry_t* lfn, lfn_part_t* part) {
//     int pos = 0;
//     for (int k = 0; k < 5; k++) {
//         uint16_t c = lfn->name1[k];
//         if (c == 0x0000) return true; // End of name
//         if (c == 0xFFFF) continue; // Padding
//         if (!is_valid_fat32_lfn_char(c)) return false;
//         part->chars[pos++] = c;
//     }
//     for (int k = 0; k < 6; k++) {
//         uint16_t c = lfn->name2[k];
//         if (c == 0x0000) return true;
//         if (c == 0xFFFF) continue;
//         if (!is_valid_fat32_lfn_char(c)) return false;
//         part->chars[pos++] = c;
//     }
//     for (int k = 0; k < 2; k++) {
//         uint16_t c = lfn->name3[k];
//         if (c == 0x0000) return true;
//         if (c == 0xFFFF) continue;
//         if (!is_valid_fat32_lfn_char(c)) return false;
//         part->chars[pos++] = c;
//     }
//     return true;
// }

// // List directory contents
// void fat32_list_dir(fat32_fs_t *fs, uint32_t cluster) {
//     block_dev_t *dev = ide_get_block_dev();
//     if (!dev) {
//         serial_printf("FAT32: No IDE device\n");
//         return;
//     }

//     uint8_t buffer[512];
//     uint16_t lfn_buffer[LFN_MAX_LEN + 1];
//     uint8_t lfn_ascii_buffer[LFN_MAX_LEN + 1];
//     lfn_part_t lfn_parts[MAX_LFN_ENTRIES];
//     int lfn_part_count = 0;
//     uint8_t lfn_checksum = 0;
//     int lfn_expected_seq = 0;

//     while (cluster != 0xFFFFFFFF) {
//         uint32_t lba = fat32_cluster_to_lba(fs, cluster);
//         for (uint32_t i = 0; i < fs->bpb.sectors_per_cluster; i++) {
//             if (ide_read_sectors(lba + i, buffer, 1) != 0) {
//                 serial_printf("FAT32: Failed to read directory sector %u\n", lba + i);
//                 return;
//             }

//             for (uint32_t j = 0; j < 512; j += 32) {
//                 if (buffer[j] == 0x00) return; // End of directory
//                 if (buffer[j] == 0xE5) continue; // Deleted entry

//                 fat32_dir_entry_t *entry = (fat32_dir_entry_t *)(buffer + j);
//                 if (entry->attributes == 0x0F) {
//                     // LFN entry
//                     fat32_lfn_entry_t *lfn = (fat32_lfn_entry_t *)(buffer + j);
//                     int seq = lfn->sequence_number & 0x1F;
//                     if (lfn->sequence_number & 0x40) {
//                         // First LFN entry
//                         lfn_part_count = 0;
//                         lfn_expected_seq = seq;
//                         lfn_checksum = lfn->checksum;
//                     }
//                     if (seq != lfn_expected_seq || lfn_part_count >= MAX_LFN_ENTRIES) {
//                         serial_printf("LFN sequence mismatch: expected %d, got %d\n", lfn_expected_seq, seq);
//                         lfn_part_count = 0;
//                         lfn_expected_seq = 0;
//                         continue;
//                     }

//                     // Store LFN part
//                     lfn_parts[lfn_part_count].seq = seq;
//                     if (!process_lfn_entry(lfn, &lfn_parts[lfn_part_count])) {
//                         serial_printf("Invalid LFN entry at seq %d\n", seq);
//                         lfn_part_count = 0;
//                         lfn_expected_seq = 0;
//                         continue;
//                     }
//                     lfn_part_count++;
//                     lfn_expected_seq--;
//                     continue;
//                 }

//                 // SFN entry
//                 char name[13];
//                 int k = 0;
//                 for (int m = 0; m < 8 && entry->name[m] != ' '; m++) name[k++] = entry->name[m];
//                 if (entry->ext[0] != ' ') {
//                     name[k++] = '.';
//                     for (int m = 0; m < 3 && entry->ext[m] != ' '; m++) name[k++] = entry->ext[m];
//                 }
//                 name[k] = '\0';

//                 // Compute LFN checksum
//                 uint8_t sfn_raw[11];
//                 memcpy(sfn_raw, entry->name, 8);
//                 memcpy(sfn_raw + 8, entry->ext, 3);
//                 uint8_t computed_checksum = compute_sfn_checksum(sfn_raw);

//                 // Assemble LFN if valid
//                 size_t lfn_pos = 0;
//                 if (lfn_part_count > 0 && lfn_expected_seq == 0 && computed_checksum == lfn_checksum) {
//                     // Concatenate LFN parts in correct order (lowest seq first)
//                     for (int p = lfn_part_count - 1; p >= 0; p--) {
//                         for (int c = 0; c < 13 && lfn_parts[p].chars[c] != 0x0000; c++) {
//                             if (lfn_pos >= LFN_MAX_LEN) break;
//                             lfn_buffer[lfn_pos++] = lfn_parts[p].chars[c];
//                         }
//                     }
//                     lfn_buffer[lfn_pos] = 0x0000; // Null terminate
//                     if (unicode_to_ascii(lfn_buffer, lfn_ascii_buffer, LFN_MAX_LEN + 1) != 0) {
//                         serial_printf("Error converting LFN to ASCII\n");
//                         lfn_pos = 0;
//                     }
//                 } else {
//                     lfn_pos = 0;
//                 }

//                 // Select final name
//                 const char *final_name = (lfn_pos > 0) ? lfn_ascii_buffer : name;
//                 if (lfn_pos > 0) {
//                     serial_printf("LFN: %s (checksum match %d)\n", lfn_ascii_buffer, computed_checksum == lfn_checksum);
//                 } else if (lfn_part_count > 0) {
//                     serial_printf("LFN rejected: seq %d, checksum match %d\n",
//                                   lfn_expected_seq, computed_checksum == lfn_checksum);
//                 }
//                 serial_printf("SFN: %s\n", name);

//                 // Handle attributes
//                 if (entry->attributes & 0x08) {
//                     serial_printf("VOL : %s\n", final_name);
//                 } else {
//                     char attr_str[5] = "    ";
//                     if (entry->attributes & 0x01) attr_str[0] = 'R';
//                     if (entry->attributes & 0x02) attr_str[1] = 'H';
//                     if (entry->attributes & 0x04) attr_str[2] = 'S';
//                     if (entry->attributes & 0x20) attr_str[3] = 'A';
//                     const char *type = (entry->attributes & 0x10) ? "DIR " : "FILE";
//                     uint32_t first_cluster = ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
//                     serial_printf("%s %s: %s, Cluster %u, Size %u\n", type, attr_str, final_name, first_cluster, entry->file_size);
//                 }

//                 lfn_part_count = 0; // Reset for next entry
//                 lfn_expected_seq = 0;
//             }
//         }
//         cluster = fat32_next_cluster(fs, cluster);
//     }
// }

// // Read file contents
// int fat32_read_file(fat32_fs_t *fs, uint32_t cluster, uint32_t size, uint8_t *buffer) {
//     block_dev_t *dev = ide_get_block_dev();
//     if (!dev) {
//         serial_printf("FAT32: No IDE device\n");
//         return -1;
//     }

//     uint32_t bytes_read = 0;
//     uint32_t cluster_size = fs->bpb.sectors_per_cluster * fs->bpb.bytes_per_sector;
//     uint8_t cluster_buffer[4096]; // Assumes max 8 sectors per cluster

//     while (cluster != 0xFFFFFFFF && bytes_read < size) {
//         if (fat32_read_cluster(fs, cluster, cluster_buffer) != 0) {
//             serial_printf("FAT32: Failed to read cluster %u\n", cluster);
//             return -1;
//         }

//         uint32_t to_copy = (size - bytes_read < cluster_size) ? (size - bytes_read) : cluster_size;
//         for (uint32_t i = 0; i < to_copy; i++) {
//             buffer[bytes_read++] = cluster_buffer[i];
//         }

//         cluster = fat32_next_cluster(fs, cluster);
//     }

//     return bytes_read;
// }