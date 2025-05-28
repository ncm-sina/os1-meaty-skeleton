#ifndef IDE_H
#define IDE_H

#include <stdint.h>
#include "pci.h"

// IDE registers (relative to base)
#define IDE_REG_DATA        0x0
#define IDE_REG_ERROR       0x1
#define IDE_REG_SECTOR_COUNT 0x2
#define IDE_REG_LBA_LOW     0x3
#define IDE_REG_LBA_MID     0x4
#define IDE_REG_LBA_HIGH    0x5
#define IDE_REG_DRIVE_HEAD  0x6
#define IDE_REG_STATUS      0x7
#define IDE_REG_COMMAND     0x7
#define IDE_REG_CONTROL     0x0 // Relative to control base

// Status bits
#define IDE_STATUS_BSY      0x80
#define IDE_STATUS_DRQ      0x08
#define IDE_STATUS_ERR      0x01

// ATA commands
#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY      0xEC

// Drive selection
#define IDE_DRIVE_MASTER    0xA0

// Default I/O bases (primary channel, legacy)
#define IDE_PRIMARY_BASE    0x1F0
#define IDE_PRIMARY_CONTROL 0x3F6

// Block device structure
typedef struct {
    int (*read_sectors)(uint32_t lba, uint8_t *buffer, uint32_t count);
    int (*write_sectors)(uint32_t lba, const uint8_t *buffer, uint32_t count);
}__attribute__((packed)) block_dev_t;

// Function prototypes
int ide_init(void);
block_dev_t *ide_get_block_dev(void);
void ide_irq_handler(void);

#endif