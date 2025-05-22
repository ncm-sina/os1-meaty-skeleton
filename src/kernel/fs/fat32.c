#include <kernel/fs/fat32.h>
// #include <kernel/drivers/ide.h>
// #include <stdlib.h>
// #include <string.h>

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

// Global FAT32 filesystem instance
static fat32_fs_t *global_fs;

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

static uint32_t fat32_read_fat_entry(fat32_fs_t *fs, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_lba = fs->fat_start_lba + (fat_offset / fs->bpb.bytes_per_sector);
    uint32_t entry_offset = fat_offset % fs->bpb.bytes_per_sector;
    uint8_t buffer[512];
    if (fs->dev->read_sectors(fat_lba, buffer, 1) != 0) return 0xFFFFFFFF;
    return *(uint32_t *)(buffer + entry_offset) & 0x0FFFFFFF;
}

static uint32_t fat32_next_cluster(fat32_fs_t *fs, uint32_t cluster) {
    uint32_t next = fat32_read_fat_entry(fs, cluster);
    if (next >= 0x0FFFFFF8) return 0xFFFFFFFF;
    return next;
}

static uint32_t fat32_cluster_to_lba(fat32_fs_t *fs, uint32_t cluster) {
    return fs->data_start_lba + ((cluster - 2) * fs->bpb.sectors_per_cluster);
}

static int fat32_read_cluster(fat32_fs_t *fs, uint32_t cluster, uint8_t *buffer) {
    uint32_t lba = fat32_cluster_to_lba(fs, cluster);
    return fs->dev->read_sectors(lba, buffer, fs->bpb.sectors_per_cluster);
}

static uint8_t compute_sfn_checksum(uint8_t *sfn) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + sfn[i];
    }
    return sum;
}

int fat32_init(fat32_fs_t *fs, uint32_t partition_lba) {
    block_dev_t *dev = ide_get_block_dev();
    if (!dev) return -EIO;
    fs->dev = dev;
    fs->partition_lba = partition_lba;

    uint8_t buffer[512];
    if (fs->dev->read_sectors(partition_lba, buffer, 1) != 0) return -EIO;

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
    global_fs = fs;
    return 0;
}

int fat32_mount(fat32_fs_t *fs) {
    return vfs_mount("/", &fat32_ops, fs);
}

static int fat32_open(struct vfs_node *node, int flags, mode_t mode) {
    if (flags & O_WRONLY) return -EACCES; // Read-only for now
    if (!global_fs) return -EIO;
    node->private_data = global_fs;
    return 0;
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
        size_t to_copy = (count - bytes_read < cluster_size - (off_t)offset) ? (count - bytes_read) : (cluster_size - (off_t)offset);
        // size_t to_copy = (count - bytes_read < cluster_size - (size_t)offset) ? (count - bytes_read) : (cluster_size - (size_t)offset);        memcpy((uint8_t *)buf + bytes_read, cluster_buffer + offset, to_copy);
        bytes_read += to_copy;
        offset = 0;
        cluster = fat32_next_cluster(fs, cluster);
    }

    return bytes_read;
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
    char temp_lfn[14]; // Temporary buffer for each LFN part (13 chars + null)

    while (cluster != 0xFFFFFFFF) {
        uint32_t lba = fat32_cluster_to_lba(fs, cluster);
        for (uint32_t i = 0; i < fs->bpb.sectors_per_cluster; i++) {
            if (fs->dev->read_sectors(lba + i, buffer, 1) != 0) {
                serial_printf("readdir: Failed to read sector %u\n", lba + i);
                return -EIO;
            }
            for (uint32_t j = 0; j < 512; j += 32) {
                if (buffer[j] == 0x00) {
                    serial_printf("readdir: End of directory\n");
                    return 0; // End of directory
                }
                if (buffer[j] == 0xE5) {
                    // serial_printf("readdir: Skipped deleted entry at offset %u\n", j);
                    continue; // Deleted entry
                }

                struct fat32_dir_entry *dir_entry = (struct fat32_dir_entry *)(buffer + j);
                if (dir_entry->attributes == 0x0F) { // LFN entry
                    struct fat32_lfn_entry *lfn = (struct fat32_lfn_entry *)(buffer + j);
                    int seq = lfn->sequence_number & 0x1F;
                    // serial_printf("readdir: LFN entry, seq=0x%x, checksum=0x%x\n", lfn->sequence_number, lfn->checksum);

                    // Start of new LFN sequence
                    if (lfn->sequence_number & 0x40) {
                        lfn_pos = 0;
                        lfn_sequence = seq;
                        lfn_checksum = lfn->checksum;
                        lfn_buffer[0] = '\0';
                        // serial_printf("readdir: Start new LFN, seq=%d\n", seq);
                    }

                    // Validate sequence
                    if (seq != lfn_sequence || lfn_checksum != lfn->checksum) {
                        serial_printf("readdir: Invalid LFN seq=%d (expected %d) or checksum=0x%x\n", seq, lfn_sequence, lfn->checksum);
                        lfn_pos = 0;
                        lfn_sequence = 0;
                        lfn_buffer[0] = '\0';
                        continue;
                    }

                    // Extract LFN characters (13 per entry)
                    int temp_pos = 0;
                    for (int k = 0; k < 5 && temp_pos < 13; k++) {
                        if (lfn->name1[k] <= 0xFF && lfn->name1[k] != 0xFFFF) {
                            temp_lfn[temp_pos++] = (char)lfn->name1[k];
                        }
                    }
                    for (int k = 0; k < 6 && temp_pos < 13; k++) {
                        if (lfn->name2[k] <= 0xFF && lfn->name2[k] != 0xFFFF) {
                            temp_lfn[temp_pos++] = (char)lfn->name2[k];
                        }
                    }
                    for (int k = 0; k < 2 && temp_pos < 13; k++) {
                        if (lfn->name3[k] <= 0xFF && lfn->name3[k] != 0xFFFF) {
                            temp_lfn[temp_pos++] = (char)lfn->name3[k];
                        }
                    }
                    temp_lfn[temp_pos] = '\0';
                    // serial_printf("readdir: LFN part: %s\n", temp_lfn);

                    // Prepend to lfn_buffer (reverse order)
                    if (lfn_pos + temp_pos < 255) {
                        memmove(lfn_buffer + temp_pos, lfn_buffer, lfn_pos + 1);
                        strncpy(lfn_buffer, temp_lfn, temp_pos);
                        lfn_pos += temp_pos;
                    } else {
                        serial_printf("readdir: LFN buffer overflow\n");
                        lfn_pos = 0;
                        lfn_sequence = 0;
                        lfn_buffer[0] = '\0';
                    }

                    lfn_sequence--;
                    continue;
                }

                // SFN entry
                if (current_index < *index) {
                    current_index++;
                    lfn_pos = 0;
                    lfn_sequence = 0;
                    lfn_buffer[0] = '\0';
                    // serial_printf("readdir: Skipping entry, current_index=%u, target=%u\n", current_index, *index);
                    continue;
                }

                // Build SFN
                char name[13];
                int k = 0;
                for (int m = 0; m < 8 && dir_entry->name[m] != ' '; m++) {
                    name[k++] = dir_entry->name[m];
                }
                if (dir_entry->ext[0] != ' ') {
                    name[k++] = '.';
                    for (int m = 0; m < 3 && dir_entry->ext[m] != ' '; m++) {
                        name[k++] = dir_entry->ext[m];
                    }
                }
                name[k] = '\0';
                // serial_printf("readdir: SFN: %s\n", name);

                // Compute SFN checksum
                uint8_t sfn_raw[11];
                memcpy(sfn_raw, dir_entry->name, 8);
                memcpy(sfn_raw + 8, dir_entry->ext, 3);
                uint8_t computed_checksum = compute_sfn_checksum(sfn_raw);
                // serial_printf("readdir: SFN checksum=0x%x, LFN checksum=0x%x\n", computed_checksum, lfn_checksum);

                // Skip volume labels
                if (dir_entry->attributes & 0x08) {
                    lfn_pos = 0;
                    lfn_sequence = 0;
                    lfn_buffer[0] = '\0';
                    // serial_printf("readdir: Skipped volume label\n");
                    continue;
                }

                // Choose name
                const char *final_name = (lfn_pos > 0 && lfn_sequence == 0 && computed_checksum == lfn_checksum) ? lfn_buffer : name;
                // serial_printf("readdir: Final name: %s\n", final_name);

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

    // serial_printf("readdir: No more entries\n");
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