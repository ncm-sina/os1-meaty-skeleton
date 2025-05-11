#ifndef DISK_H
#define DISK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ahci.h"

typedef struct {
    uint8_t status;
    uint8_t start_chs[3];
    uint8_t type;
    uint8_t end_chs[3];
    uint32_t start_lba;
    uint32_t size;
} __attribute__((packed)) PartitionEntry;

typedef struct {
    uint8_t boot_code[446];
    PartitionEntry partitions[4];
    uint16_t signature;
} __attribute__((packed)) MBR;

int disk_create_partition(Disk* disk, uint32_t start_lba, uint32_t size);
int disk_install_grub(Disk* disk, uint32_t partition_lba);

#endif // DISK_H