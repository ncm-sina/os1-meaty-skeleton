#ifndef AHCI_H
#define AHCI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HBA_PORT_DET_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE 0x1
#define SATA_SIG_ATA 0x00000101
#define SATA_SIG_ATAPI 0xEB140101

// Command types for ahci_read_sectors
#define AHCI_CMD_READ     0 // Read data (HDD: READ DMA, CDROM: READ(10))
#define AHCI_CMD_IDENTIFY 1 // Identify device (HDD: IDENTIFY DEVICE, CDROM: IDENTIFY PACKET DEVICE)


typedef struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t reserved0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t reserved1[11];
    uint32_t vendor[4];
} __attribute__((packed)) HBA_PORT;

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_ports;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t reserved[0x74];
    uint8_t vendor[0x60];
    uint8_t ports[0];
} __attribute__((packed)) HBA_MEM;

typedef struct {
    uint8_t cfl:5;
    uint8_t a:1;
    uint8_t w:1;
    uint8_t p:1;
    uint8_t r:1;
    uint8_t b:1;
    uint8_t c:1;
    uint8_t reserved0:1;
    uint8_t pmp:4;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t reserved1[4];
} __attribute__((packed)) HBA_CMD_HEADER;

typedef struct {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t reserved[48];
    struct {
        uint32_t dba;
        uint32_t dbau;
        uint32_t reserved0;
        uint32_t dbc:22;
        uint32_t reserved1:9;
        uint32_t i:1;
    } __attribute__((packed)) prdt_entry[8];
} __attribute__((packed)) HBA_CMD_TBL;

typedef struct {
    uint32_t port_index;
    int type;           // 0: ATA (HDD), 1: ATAPI (CDROM)
    int content_type;   // 0: HDD (FAT32/raw), 1: ISO9660
    uint32_t total_sectors;
    HBA_PORT* port;
} Disk;

#define SATA_SIG_ATA    0x00000101
#define SATA_SIG_ATAPI  0xEB140101
#define HBA_PORT_DET_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE  0x1


// typedef struct {
//     uint32_t cap;
//     uint32_t ghc;
//     uint32_t is;
//     uint32_t pi;
//     uint32_t vs;
//     uint32_t ccc_ctl;
//     uint32_t ccc_ports;
//     uint32_t em_loc;
//     uint32_t em_ctl;
//     uint32_t cap2;
//     uint32_t bohc;
//     uint8_t reserved[0x74];
//     uint8_t vendor[0x60];
//     uint8_t ports[0];
// } __attribute__((packed)) HBA_MEM;

// typedef struct {
//     uint32_t clb;
//     uint32_t clbu;
//     uint32_t fb;
//     uint32_t fbu;
//     uint32_t is;
//     uint32_t ie;
//     uint32_t cmd;
//     uint32_t reserved0;
//     uint32_t tfd;
//     uint32_t sig;
//     uint32_t ssts;
//     uint32_t sctl;
//     uint32_t serr;
//     uint32_t sact;
//     uint32_t ci;
//     uint32_t sntf;
//     uint32_t fbs;
//     uint32_t reserved1[11];
//     uint32_t vendor[4];
// } __attribute__((packed)) HBA_PORT;

// typedef struct {
//     uint8_t cfl:5;
//     uint8_t a:1;
//     uint8_t w:1;
//     uint8_t p:1;
//     uint8_t r:1;
//     uint8_t b:1;
//     uint8_t c:1;
//     uint8_t reserved0:1;
//     uint8_t pmp:4;
//     uint16_t prdtl;
//     uint32_t prdbc;
//     uint32_t ctba;
//     uint32_t ctbau;
//     uint32_t reserved1[4];
// } __attribute__((packed)) HBA_CMD_HEADER;

// typedef struct {
//     uint8_t cfis[64];
//     uint8_t acmd[16];
//     uint8_t reserved[48];
//     struct {
//         uint32_t dba;
//         uint32_t dbau;
//         uint32_t reserved0;
//         uint32_t dbc:22;
//         uint32_t reserved1:9;
//         uint32_t i:1;
//     } __attribute__((packed)) prdt_entry[8];
// } __attribute__((packed)) HBA_CMD_TBL;


// typedef struct {
//     uint32_t port_index; // AHCI port number (0-31)
//     int type;
//     uint32_t total_sectors;
//     HBA_PORT* port;
// } Disk;
// typedef struct {
//     HBA_PORT* port;
//     uint32_t total_sectors;
//     uint8_t type; // 0: HDD, 1: CDROM
// } Disk;

void ahci_init(void);
int ahci_get_disks(Disk* disks, int max_disks);
int ahci_read_sectors(Disk* disk, uint32_t lba, uint32_t count, void* buffer);
int ahci_read_sectors2(Disk* disk, uint32_t lba, uint32_t count, void* buffer, int command);

int ahci_write_sectors(Disk* disk, uint32_t lba, uint32_t count, const void* buffer);
int ahci_write_sectors2(Disk* disk, uint32_t lba, uint32_t count, void* buffer);

#endif // AHCI_H