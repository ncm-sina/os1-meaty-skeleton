#include "ahci.h"
#include "pci.h"
#include "memory.h"
#include <string.h>

static HBA_MEM* hba_mem;
static Disk disks[8];
static int num_disks;

static void port_rebase(HBA_PORT* port) {
    port->cmd &= ~(1 << 0); // Stop ST
    port->cmd &= ~(1 << 4); // Stop FRE
    while (port->cmd & (1 << 15) || port->cmd & (1 << 14));

    if (port->clb) kfree((void*)port->clb);
    if (port->fb) kfree((void*)port->fb);

    port->clb = (uint32_t)kmalloc(1024);
    port->clbu = 0;
    port->fb = (uint32_t)kmalloc(256);
    port->fbu = 0;
    memset((void*)port->clb, 0, 1024);
    memset((void*)port->fb, 0, 256);

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)port->clb;
    for (int i = 0; i < 32; i++) {
        cmd_header[i].ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
        cmd_header[i].ctbau = 0;
        memset((void*)cmd_header[i].ctba, 0, sizeof(HBA_CMD_TBL));
    }

    port->serr = 0xFFFFFFFF;
    port->is = 0xFFFFFFFF;
    port->ie = 0xFFFFFFFF;
    port->cmd |= (1 << 4); // Enable FRE
    port->cmd |= (1 << 0); // Enable ST
}

static int probe_device(HBA_PORT* port, Disk* disk) {
    // Probe device for sector count
    static int i=0;
    i++;
    uint8_t buffer[512];
    disk->port = port;
    if (ahci_read_sectors2(disk, 0, 1, buffer, AHCI_CMD_IDENTIFY) == 0) {
        uint16_t* identify = (uint16_t*)buffer;
        if (disk->type == 0) {
            // HDD: Use words 100-103 for 48-bit LBA capacity
            uint64_t sectors = ((uint64_t)identify[100] |
                                ((uint64_t)identify[101] << 16) |
                                ((uint64_t)identify[102] << 32) |
                                ((uint64_t)identify[103] << 48));
            disk->total_sectors = sectors ? sectors : 10000;
        } else {
            // CDROM: Use words 100-103 if available, or fallback
            uint64_t sectors = ((uint64_t)identify[100] |
                                ((uint64_t)identify[101] << 16) |
                                ((uint64_t)identify[102] << 32) |
                                ((uint64_t)identify[103] << 48));
            disk->total_sectors = sectors ? sectors : 20480; // ~400MB default
        }
    } else {
        printf("\n Port %d: IDENTIFY failed,type=0x%08x TFD=0x%08x\n",i-1, disk->type, port->tfd);
        disk->total_sectors = (disk->type == 0) ? 10000 : 20480;
    }
    return 0;
    // // Simplified IDENTIFY DEVICE for ATA
    // uint8_t buffer[512];
    // disk->port = port;
    // disk->type = 0; // Assume HDD
    // disk->total_sectors = 0;
    // if (ahci_read_sectors(disk, 0, 1, buffer) == 0) {
    //     uint16_t* identify = (uint16_t*)buffer;
    //     uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
    //                        ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
    //     disk->total_sectors = sectors ? sectors : 10000000;
    //     return 0;
    // }
    // return -1;
}

void ahci_init(void) {
    uint8_t bus, slot, func;
    if (!pci_find_device(0x01, 0x06, &bus, &slot, &func)) {
        printf("Error: No AHCI controller found\n");
        return;
    }

    uint32_t bar5 = pci_read_bar(bus, slot, func, 5);
    if (!bar5) {
        printf("Error: Invalid BAR5\n");
        return;
    }
    hba_mem = (HBA_MEM*)bar5;

    // printf("AHCI init: hba_mem=%08x, cap=%08x, pi=%08x\n",
    //        hba_mem, hba_mem->cap, hba_mem->pi);

    // Reset HBA
    hba_mem->ghc |= (1 << 0); // Global reset
    uint32_t timeout = 1000000;
    while ((hba_mem->ghc & (1 << 0)) && timeout--) asm volatile("pause");
    hba_mem->ghc |= (1 << 31); // Enable AHCI mode

    num_disks = 0;
    for (int i = 0; i < 32 && num_disks < 8; i++) {
        if (hba_mem->pi & (1 << i)) {
            HBA_PORT* port = (HBA_PORT*)((uint8_t*)hba_mem + 0x100 + i * 0x80);
            uint32_t ssts = port->ssts;
            if ((ssts & 0x0F) == HBA_PORT_DET_PRESENT && (ssts >> 8 & 0x0F) == HBA_PORT_IPM_ACTIVE) {
                port->sctl = (port->sctl & ~(0x7 << 8)) | (0x3 << 8);
                port_rebase(port);
                timeout = 1000000;
                while (timeout--) asm volatile("pause");

                disks[num_disks].port = port;
                disks[num_disks].port_index = i;
                // ISO on port 1 treated as ATA
                disks[num_disks].type = (port->sig == SATA_SIG_ATA || (port->sig == SATA_SIG_ATAPI && i == 1)) ? 0 : (port->sig == SATA_SIG_ATAPI) ? 1 : -1;
                disks[num_disks].content_type = (/*port->sig == SATA_SIG_ATAPI &&*/ i == 1) ? 1 : 0;
                if (disks[num_disks].type == -1) {
                    printf("Port %d: Unknown device, sig=0x%08x\n", i, port->sig);
                    continue;
                }
                disks[num_disks].total_sectors = 0;

                uint8_t buffer[512];
                printf("Port %d: Attempting IDENTIFY, type=%d, content_type=%d\n", i, disks[num_disks].type, disks[num_disks].content_type);
                if (ahci_read_sectors2(&disks[num_disks], 0, 1, buffer, AHCI_CMD_IDENTIFY) == 0) {
                    uint16_t* identify = (uint16_t*)buffer;
                    uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
                                       ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
                    // Adjust for content_type
                    disks[num_disks].total_sectors = sectors ? (disks[num_disks].content_type == 1 ? sectors /*/ 4*/ : sectors) : (disks[num_disks].content_type == 1 ? 204800 : 10000000);
                    printf("Port %d: IDENTIFY success, sectors=%llu\n", i, sectors);
                } else {
                    printf("Port %d: IDENTIFY failed, TFD=0x%08x, SERR=0x%08x\n", i, port->tfd, port->serr);
                    disks[num_disks].total_sectors = (disks[num_disks].content_type == 1 ? 204800 : 10000000);
                }
                print_hba_port(&disks[num_disks]);
                printf("%d>port=%08x,T=%d,CT=%d,ssts=%08x,sig=%08x,sects=%u\n",
                       num_disks, disks[num_disks].port, disks[num_disks].type, disks[num_disks].content_type, ssts, port->sig,
                       disks[num_disks].total_sectors);
                num_disks++;
            }
        }
    }
    printf("AHCI init complete, found %d disks\n", num_disks);

    // // Copy ISO to HDD if both disks are present
    // if (num_disks >= 2 && disks[0].type == 0 && disks[1].type == 0 && disks[1].content_type == 1) {
    //     copy_iso_to_hdd(&disks[1], &disks[0]);
    // }
}

int ahci_get_disks(Disk* disks_out, int max_disks) {
    int count = (num_disks < max_disks) ? num_disks : max_disks;
    memcpy(disks_out, disks, count * sizeof(Disk));
    return count;
}

int ahci_read_sectors(Disk* disk, uint32_t lba, uint32_t count, void* buffer) {
    return ahci_read_sectors2(disk, lba, count, buffer, AHCI_CMD_READ);
}

int ahci_read_sectors2(Disk* disk, uint32_t lba, uint32_t count, void* buffer, int command) {
    printf("ahci_read: port=%08x, type=%d, content_type=%d, lba=%u, count=%u, buffer=%08x, cmd=%d\n",
           disk->port, disk->type, disk->content_type, lba, count, buffer, command);

    HBA_PORT* port = disk->port;
    port->is = 0xFFFFFFFF; // Clear interrupt status

    // Find free command slot
    int slot = -1;
    for (int i = 0; i < 32; i++) {
        if (!(port->ci & (1 << i))) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("Error: No free command slot, ci=0x%08x\n", port->ci);
        return -1;
    }

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
    cmd_header->cfl = 5; // 20-byte H2D FIS
    cmd_header->a = (disk->type == 1); // ATAPI bit
    cmd_header->w = 0; // Read operation
    cmd_header->prdtl = 1; // Single PRD

    cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
    if (!cmd_header->ctba) {
        printf("Error: kmalloc failed\n");
        return -1;
    }
    cmd_header->ctbau = 0; // 32-bit addressing

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)cmd_header->ctba;
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));

    // Setup PRD
    uint32_t sector_size = (disk->content_type == 1) ? 2048 : 512;
    uint32_t transfer_bytes = (command == AHCI_CMD_IDENTIFY) ? 512 : count * sector_size;
    cmd_tbl->prdt_entry[0].dba = (uint32_t)buffer;
    cmd_tbl->prdt_entry[0].dbau = 0;
    cmd_tbl->prdt_entry[0].dbc = transfer_bytes - 1;
    cmd_tbl->prdt_entry[0].i = 1;

    if (disk->type == 1) {
        // ATAPI (CDROM)
        cmd_tbl->cfis[0] = 0x27; // H2D FIS
        cmd_tbl->cfis[1] = 0x80; // Command bit
        cmd_tbl->cfis[2] = 0xA0; // PACKET command
        memset(cmd_tbl->acmd, 0, 16);
        if (command == AHCI_CMD_IDENTIFY) {
            cmd_tbl->acmd[0] = 0xA1; // IDENTIFY PACKET DEVICE
            cmd_tbl->acmd[4] = 0x01; // Request 512 bytes
            printf("ATAPI IDENTIFY: acmd=[%02x %02x %02x %02x %02x]\n",
                   cmd_tbl->acmd[0], cmd_tbl->acmd[1], cmd_tbl->acmd[2], cmd_tbl->acmd[3], cmd_tbl->acmd[4]);
        } else {
            cmd_tbl->acmd[0] = 0x28; // READ(10)
            cmd_tbl->acmd[2] = (lba >> 24) & 0xFF;
            cmd_tbl->acmd[3] = (lba >> 16) & 0xFF;
            cmd_tbl->acmd[4] = (lba >> 8) & 0xFF;
            cmd_tbl->acmd[5] = lba & 0xFF;
            cmd_tbl->acmd[7] = (count >> 8) & 0xFF;
            cmd_tbl->acmd[8] = count & 0xFF;
            printf("ATAPI READ(10): acmd=[%02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
                   cmd_tbl->acmd[0], cmd_tbl->acmd[1], cmd_tbl->acmd[2], cmd_tbl->acmd[3],
                   cmd_tbl->acmd[4], cmd_tbl->acmd[5], cmd_tbl->acmd[6], cmd_tbl->acmd[7],
                   cmd_tbl->acmd[8]);
        }
    } else {
        // ATA (HDD or ISO-as-HDD)
        cmd_tbl->cfis[0] = 0x27; // H2D FIS
        cmd_tbl->cfis[1] = 0x80; // Command bit
        cmd_tbl->cfis[2] = (command == AHCI_CMD_IDENTIFY) ? 0xEC : 0xC8;
        if (command == AHCI_CMD_READ) {
            cmd_tbl->cfis[4] = lba & 0xFF;
            cmd_tbl->cfis[5] = (lba >> 8) & 0xFF;
            cmd_tbl->cfis[6] = (lba >> 16) & 0xFF;
            cmd_tbl->cfis[7] = (lba >> 24) & 0xFF;
            cmd_tbl->cfis[12] = count & 0xFF;
            cmd_tbl->cfis[13] = (count >> 8) & 0xFF;
        } else {
            cmd_tbl->cfis[4] = 0;
            cmd_tbl->cfis[5] = 0;
            cmd_tbl->cfis[6] = 0;
            cmd_tbl->cfis[7] = 0;
            cmd_tbl->cfis[12] = 0;
            cmd_tbl->cfis[13] = 0;
        }
    }

    print_ahci_command_state(port, cmd_header, cmd_tbl, disk->type == 1);
    port->ci = 1 << slot;

    uint32_t timeout = 1000000;
    while ((port->ci & (1 << slot)) && !(port->is & (1 << 30)) && timeout--) {
        asm volatile("pause");
    }

    int result = 0;
    if (timeout == 0) {
        printf("Error: Command timeout, TFD=0x%08x, SERR=0x%08x\n", port->tfd, port->serr);
        print_hba_port(port);
        result = -1;
    } else if (port->is & (1 << 30)) {
        printf("Error: Device fault, TFD=0x%08x, SERR=0x%08x\n", port->tfd, port->serr);
        print_hba_port(port);
        result = -1;
    }

    printf("timeout=%u, ci=0x%08x, slot=0x%08x\n", timeout, port->ci, 1 << slot);
    kfree((void*)cmd_header->ctba);
    return result;
}

int ahci_write_sectors2(Disk* disk, uint32_t lba, uint32_t count, void* buffer) {
    HBA_PORT* port = disk->port;
    port->is = 0xFFFFFFFF;

    int slot = 0;
    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
    cmd_header->cfl = 5;
    cmd_header->a = (disk->type == 1);
    cmd_header->w = 1;
    cmd_header->prdtl = (count + 7) / 8;
    cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
    cmd_header->ctbau = 0;

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));
    const uint8_t* buf_ptr = buffer;
    uint32_t sector_size = (disk->content_type == 1) ? 2048 : 512;
    // uint32_t transfer_bytes = count * sector_size;
    for (uint32_t i = 0; i < cmd_header->prdtl; i++) {
        uint32_t sectors = (count > 8) ? 8 : count;
        cmd_tbl->prdt_entry[i].dba = (uint32_t)buf_ptr;
        cmd_tbl->prdt_entry[i].dbau = 0;
        cmd_tbl->prdt_entry[i].dbc = sectors * sector_size - 1;
        cmd_tbl->prdt_entry[i].i = 1;
        buf_ptr += sectors * sector_size;
        count -= sectors;
    }

    cmd_tbl->cfis[0] = 0x27;
    cmd_tbl->cfis[1] = 0x80;
    cmd_tbl->cfis[2] = (disk->type == 0) ? 0xCA : 0x35;
    cmd_tbl->cfis[4] = lba & 0xFF;
    cmd_tbl->cfis[5] = (lba >> 8) & 0xFF;
    cmd_tbl->cfis[6] = (lba >> 16) & 0xFF;
    cmd_tbl->cfis[7] = (lba >> 24) & 0xFF;
    cmd_tbl->cfis[12] = count & 0xFF;
    cmd_tbl->cfis[13] = (count >> 8) & 0xFF;

    port->ci = 1 << slot;
    while ((port->ci & (1 << slot)) && !(port->is & (1 << 30))) {
        asm volatile("pause");
    }
    int result = (port->is & (1 << 30)) ? -1 : 0;
    kfree((void*)cmd_header->ctba);
    return result;
}

int ahci_write_sectors(Disk* disk, uint32_t lba, uint32_t count, const void* buffer) {
    printf("ahci_write: port=%08x, type=%d, content_type=%d, lba=%u, count=%u, buffer=%08x\n",
           disk->port, disk->type, disk->content_type, lba, count, buffer);

    HBA_PORT* port = disk->port;
    port->is = 0xFFFFFFFF;

    int slot = -1;
    for (int i = 0; i < 32; i++) {
        if (!(port->ci & (1 << i))) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("Error: No free command slot, ci=0x%08x\n", port->ci);
        return -1;
    }

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
    cmd_header->cfl = 5;
    cmd_header->a = 0; // Always ATA
    cmd_header->w = 1; // Write operation
    cmd_header->prdtl = 1;

    cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
    if (!cmd_header->ctba) {
        printf("Error: kmalloc failed\n");
        return -1;
    }
    cmd_header->ctbau = 0;

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)cmd_header->ctba;
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));
    const uint8_t* buf_ptr = buffer;

    uint32_t sector_size = (disk->content_type == 1) ? 2048 : 512;
    uint32_t transfer_bytes = count * sector_size;
    cmd_tbl->prdt_entry[0].dba = (uint32_t)buf_ptr;
    cmd_tbl->prdt_entry[0].dbau = 0;
    cmd_tbl->prdt_entry[0].dbc = transfer_bytes - 1;
    cmd_tbl->prdt_entry[0].i = 1;

    cmd_tbl->cfis[0] = 0x27; // H2D FIS
    cmd_tbl->cfis[1] = 0x80; // Command bit
    cmd_tbl->cfis[2] = 0xCA; // WRITE DMA
    cmd_tbl->cfis[4] = lba & 0xFF;
    cmd_tbl->cfis[5] = (lba >> 8) & 0xFF;
    cmd_tbl->cfis[6] = (lba >> 16) & 0xFF;
    cmd_tbl->cfis[7] = (lba >> 24) & 0xFF;
    cmd_tbl->cfis[12] = count & 0xFF;
    cmd_tbl->cfis[13] = (count >> 8) & 0xFF;

    print_ahci_command_state(port, cmd_header, cmd_tbl, 0);
    port->ci = 1 << slot;

    uint32_t timeout = 1000000;
    while ((port->ci & (1 << slot)) && !(port->is & (1 << 30)) && (timeout--)>10) {
        asm volatile("pause");
    }

    int result = 0;
    if (timeout <= 20) {
        printf("Error: Command timeout, TFD=0x%08x, SERR=0x%08x\n", port->tfd, port->serr);
        print_hba_port(port);
        result = -1;
    } else if (port->is & (1 << 30)) {
        printf("Error: Device fault, TFD=0x%08x, SERR=0x%08x\n", port->tfd, port->serr);
        print_hba_port(port);
        result = -1;
    }

    printf("timeout=%u, ci=0x%08x, slot=0x%08x\n", timeout, port->ci, 1 << slot);
    kfree((void*)cmd_header->ctba);
    return result;
}