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

// void ahci_init(void) {
//     uint8_t bus, slot, func;
//     if (!pci_find_device(0x01, 0x06, &bus, &slot, &func)) {
//         printf("Error: No AHCI controller found\n");
//         return;
//     }

//     uint32_t bar5 = pci_read_bar(bus, slot, func, 5);
//     if (!bar5) {
//         printf("Error: Invalid BAR5\n");
//         return;
//     }
//     hba_mem = (HBA_MEM*)bar5;

//     // Enable AHCI and reset HBA
//     hba_mem->ghc |= (1 << 31); // AE (AHCI Enable)
//     hba_mem->ghc |= (1 << 0);  // HR (HBA Reset)
//     int32_t timeout = 1000000;
//     while (hba_mem->ghc & (1 << 0) && timeout--) {
//         asm volatile("pause");
//     }
//     if (hba_mem->ghc & (1 << 0)) {
//         printf("Error: AHCI reset timed out\n");
//         return;
//     }

//     num_disks = 0;
//     for (int i = 0; i < 32 && num_disks < 8; i++) {
//         if (hba_mem->pi & (1 << i)) {
//             HBA_PORT* port = (HBA_PORT*)((uint8_t*)hba_mem + 0x100 + i * 0x80);
//             uint32_t ssts = port->ssts;
//             printf("Port %d: ssts=0x%x, sig=0x%x\n", i, ssts, port->sig);
//             if ((ssts & 0x0F) == HBA_PORT_DET_PRESENT &&
//                 (ssts >> 8 & 0x0F) == HBA_PORT_IPM_ACTIVE) {
//                 // Reset port
//                 port->sctl = (port->sctl & ~(0x7 << 8)) | (0x3 << 8); // Gen3 speed
//                 port_rebase(port);

//                 // Store disk info
//                 disks[num_disks].port = port;
//                 disks[num_disks].total_sectors = 0;
//                 if (probe_device(port, &disks[num_disks]) == 0) {
//                     disks[num_disks].type = 0; // HDD
//                 } else {
//                     disks[num_disks].type = 1; // Assume CDROM if IDENTIFY fails
//                     disks[num_disks].total_sectors = 0;
//                 }
//                 printf("Disk %d: port=%d, type=%d, sectors=%u\n",
//                        num_disks, i, disks[num_disks].type, disks[num_disks].total_sectors);

//                 // Probe device with IDENTIFY command
//                 // uint8_t buffer[512];
//                 // int identify_result = -1;
//                 // if (port->sig == SATA_SIG_ATAPI) {
//                 //     // ATAPI IDENTIFY PACKET DEVICE
//                 //     // Note: Requires ATAPI packet command (not implemented yet)
//                 //     disks[num_disks].type = 1; // CDROM
//                 // } else {
//                 //     // Try ATA IDENTIFY DEVICE
//                 //     identify_result = ahci_read_sectors(&disks[num_disks], 0, 1, buffer);
//                 //     disks[num_disks].type = 0; // Assume HDD
//                 // }

//                 // Set type and sector count
//                 // if (port->sig == SATA_SIG_ATA || identify_result == 0) {
//                 //     disks[num_disks].type = 0; // Confirmed HDD
//                 //     if (identify_result == 0) {
//                 //         uint16_t* identify = (uint16_t*)buffer;
//                 //         uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
//                 //                            ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
//                 //         disks[num_disks].total_sectors = sectors ? sectors : 10000000;
//                 //     }
//                 // } else if (port->sig == SATA_SIG_ATAPI) {
//                 //     disks[num_disks].type = 1; // CDROM
//                 // } else {
//                 //     printf("Port %d: Unknown signature, assuming HDD, sig=0x%x\n", i, port->sig);
//                 //     disks[num_disks].type = 0; // Fallback to HDD
//                 //     disks[num_disks].total_sectors = 10000000; // Default
//                 // }

//                 printf("Disk %d: port=%d, type=%d, sectors=%u\n",
//                        num_disks, i, disks[num_disks].type, disks[num_disks].total_sectors);
//                 num_disks++;
//             }
//         }
//     }
// }

// void ahci_init(void) {
//     // Assume hba_mem is set via PCI BAR5
//     printf("AHCI init: hba_mem=%08x, cap=%08x, pi=%08x\n",
//            hba_mem, hba_mem->cap, hba_mem->pi);

//     // Reset HBA
//     hba_mem->ghc |= (1 << 0); // Global reset
//     uint32_t timeout = 1000000;
//     while ((hba_mem->ghc & (1 << 0)) && timeout--) asm volatile("pause");
//     hba_mem->ghc |= (1 << 31); // Enable AHCI mode

//     num_disks = 0;
//     for (int i = 0; i < 32 && num_disks < 8; i++) {
//         if (hba_mem->pi & (1 << i)) {
//             HBA_PORT* port = (HBA_PORT*)((uint8_t*)hba_mem + 0x100 + i * 0x80);
//             uint32_t ssts = port->ssts;
//             if ((ssts & 0x0F) == HBA_PORT_DET_PRESENT && (ssts >> 8 & 0x0F) == HBA_PORT_IPM_ACTIVE) {
//                 // Configure port
//                 port->sctl = (port->sctl & ~(0x7 << 8)) | (0x3 << 8); // Speed Gen3
//                 port_rebase(port);
//                 timeout = 1000000;
//                 while (timeout--) asm volatile("pause");

//                 disks[num_disks].port = port;
//                 disks[num_disks].port_index = i;
//                 disks[num_disks].type = (port->sig == SATA_SIG_ATA) ? 0 : (port->sig == SATA_SIG_ATAPI) ? 1 : -1;
//                 if (disks[num_disks].type == -1) {
//                     printf("Port %d: Unknown device, sig=0x%08x\n", i, port->sig);
//                     continue;
//                 }
//                 disks[num_disks].total_sectors = 0;

//                 uint8_t buffer[512];
//                 printf("Port %d: Attempting IDENTIFY, type=%d\n", i, disks[num_disks].type);
//                 if (ahci_read_sectors2(&disks[num_disks], 0, 1, buffer, AHCI_CMD_IDENTIFY) == 0) {
//                     uint16_t* identify = (uint16_t*)buffer;
//                     uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
//                                        ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
//                     disks[num_disks].total_sectors = sectors ? sectors : (disks[num_disks].type == 1 ? 204800 : 10000000);
//                     printf("Port %d: IDENTIFY success, sectors=%llu\n", i, sectors);
//                 } else {
//                     printf("Port %d: IDENTIFY failed, TFD=0x%08x, SERR=0x%08x\n", i, port->tfd, port->serr);
//                     disks[num_disks].total_sectors = (disks[num_disks].type == 1 ? 204800 : 10000000);
//                 }
//                 print_hba_port(port);
//                 printf("%d>port=%08x,T=%d,ssts=%08x,sig=%08x,sects=%u\n",
//                        num_disks, disks[num_disks].port, disks[num_disks].type, ssts, port->sig,
//                        disks[num_disks].total_sectors);
//                 num_disks++;
//             }
//         }
//     }
//     printf("AHCI init complete, found %d disks\n", num_disks);
// }

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
    // printf("\nAHCI init: hba_mem=%08x, cap=%08x, pi=%08x\n",
    //        hba_mem, hba_mem->cap, hba_mem->pi);

    hba_mem->ghc |= (1 << 31);
    hba_mem->ghc |= (1 << 0);
    int32_t timeout = 1000000; 
    while (hba_mem->ghc & (1 << 0) && timeout--) {
        asm volatile("pause");
    }
    if (hba_mem->ghc & (1 << 0)) {
        printf("Error: AHCI reset timed out\n");
        return;
    }

    num_disks = 0;
    for (int i = 0; i < 32 && num_disks < 8; i++) {
        if (hba_mem->pi & (1 << i)) {
            HBA_PORT* port = (HBA_PORT*)((uint8_t*)hba_mem + 0x100 + i * 0x80);
            uint32_t ssts = port->ssts;
            // printf("Port %d: ssts=0x%08x, sig=0x%08x\n", i, ssts, port->sig);
            
            if ((ssts & 0x0F) == HBA_PORT_DET_PRESENT &&
            (ssts >> 8 & 0x0F) == HBA_PORT_IPM_ACTIVE) {
                port->sctl = (port->sctl & ~(0x7 << 8)) | (0x3 << 8);
                port_rebase(port);

                int32_t timeout=1000000;
                while(timeout--){
                    asm volatile("pause"); // Reduce CPU usage        
                }

                printf("P2ort %d: ssts=0x%08x, sig=0x%08x\n", i, ssts, port->sig);
                
                disks[num_disks].port = port; // and the port
                disks[num_disks].port_index = i; // Store port index
                // disks[num_disks].type = (port->sig == SATA_SIG_ATA) ? 0 : (port->sig == SATA_SIG_ATAPI) ? 1 : -1;
                // Use explicit signatures
                if (port->sig == SATA_SIG_ATA) { // 0x00000101
                    disks[num_disks].type = 0; // HDD
                } else if (port->sig == SATA_SIG_ATAPI) { // 0xEB140101
                    disks[num_disks].type = 1; // CDROM
                } else {
                    printf("Port %d: Unknown device type, sig=0x%08x\n", i, port->sig);
                    continue; // Skip unknown devices
                }
                disks[num_disks].total_sectors = 0;

                // will try to find out total sectors of the disk
                // probe_device(port, &disks[num_disks]);
                uint8_t buffer[512];
                printf("\nPort %d: Attempting IDENTIFY, type=%d\n", i, disks[num_disks].type);
                if (ahci_read_sectors2(&disks[num_disks], 0, 1, buffer, AHCI_CMD_IDENTIFY) == 0) {
                    uint16_t* identify = (uint16_t*)buffer;
                    uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
                                       ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
                    disks[num_disks].total_sectors = sectors ? sectors : (disks[num_disks].type == 1 ? 204800 : 34567);// fake secrot count
                    printf("Port %d: IDENTIFY success, sectors=%d\n", i, sectors);
                } else {
                    printf("Port %d: IDENTIFY failed, TFD=0x%08x, SERR=0x%08x\n", i, port->tfd, port->serr);
                    disks[num_disks].total_sectors = (disks[num_disks].type == 1 ? 204800 : 34567);// fake secrot count
                }
                // print_hba_port(port);
                printf("%d>port=%08x,T=%d,ssts=%08x,sig=%08x,sects=%u\n",
                       num_disks, disks[num_disks].port, disks[num_disks].type, ssts, port->sig,
                       disks[num_disks].total_sectors);
                num_disks++;

                // // Skip IDENTIFY for CDROMs (use ATAPI IDENTIFY later if needed)
                // if (disks[num_disks].type == 0) {
                //     uint8_t buffer[512];
                //     if (ahci_read_sectors(&disks[num_disks], 0, 1, buffer) == 0) {
                //         uint16_t* identify = (uint16_t*)buffer;
                //         uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
                //                            ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
                //         disks[num_disks].total_sectors = sectors ? sectors : 10000000;
                //     }
                // }
                // printf("\n%d>port=%08x,T=%d,ssts=%08x,sig=%08x,sects=%u",
                //         num_disks, disks[num_disks].port, disks[num_disks].type, ssts, port->sig, disks[num_disks].total_sectors);
                // num_disks++;
            }
        }
    }
}

// void ahci_init(void) {
//     uint8_t bus, slot, func;
//     if (!pci_find_device(0x01, 0x06, &bus, &slot, &func)) {
//         return;
//     }
//     // printf(" 2 ");

//     uint32_t bar5 = pci_read_bar(bus, slot, func, 5);
//     // printf("BAR5: 0x%08x\n", bar5);
//     hba_mem = (HBA_MEM*)bar5;
    
//     // printf(" 3 ");
    
//     // printf("GHC before: 0x%08x\n", hba_mem->ghc);
//     hba_mem->ghc |= (1 << 31);
//     hba_mem->ghc |= (1 << 0);
//     // printf("GHC after: 0x%08x\n", hba_mem->ghc);

//     int32_t timeout = 1000000; // ~1 second (adjust based on CPU speed)
//     while (hba_mem->ghc & (1 << 0) && timeout--) {
//         asm volatile("pause"); // Reduce CPU usage
//     }
//     if (hba_mem->ghc & (1 << 0)) {
//         printf("Error: AHCI reset timed out\n");
//         return;
//     }
//     // printf(" 1.4 ");

//     num_disks = 0;
//     for (int i = 0; i < 32 && num_disks < 8; i++) {
//         if (hba_mem->pi & (1 << i)) {
//             HBA_PORT* port = (HBA_PORT*)((uint8_t*)hba_mem + 0x100 + i * 0x80);
//             uint32_t ssts = port->ssts;
//             printf("Port %d: ssts=0x%08x, sig=0x%08x\n", i, ssts, port->sig);
//             if ((ssts & 0x0F) == HBA_PORT_DET_PRESENT &&
//                 (ssts >> 8 & 0x0F) == HBA_PORT_IPM_ACTIVE) {
//                 port->sctl = (port->sctl & ~(0x7 << 8)) | (0x3 << 8); // Set speed to Gen3
//                 port_rebase(port);
//                 printf("P2ort %d: ssts=0x%08x, sig=0x%08x\n", i, ssts, port->sig);
//                 disks[num_disks].port = port;
//                 disks[num_disks].type = (port->sig == SATA_SIG_ATA) ? 0 : 1;
//                 disks[num_disks].total_sectors = 0;

//                 uint8_t buffer[512];
//                 if (ahci_read_sectors(&disks[num_disks], 0, 1, buffer) == 0) {
//                     uint16_t* identify = (uint16_t*)buffer;
//                     uint64_t sectors = ((uint64_t)identify[100] | ((uint64_t)identify[101] << 16) |
//                                        ((uint64_t)identify[102] << 32) | ((uint64_t)identify[103] << 48));
//                     disks[num_disks].total_sectors = sectors ? sectors : 10000000;
//                 }
//                 num_disks++;
//             }
//         }
//     }
// }

int ahci_get_disks(Disk* disks_out, int max_disks) {
    int count = (num_disks < max_disks) ? num_disks : max_disks;
    memcpy(disks_out, disks, count * sizeof(Disk));
    return count;
}

int ahci_read_sectors2(Disk* disk, uint32_t lba, uint32_t count, void* buffer, int command) {
    // printf("ahci_read: port=%08x, type=%d, lba=%u, count=%u, buffer=%08x, cmd=%d\n",
    //        disk->port, disk->type, lba, count, buffer, command);

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
    }else{
        // printf("-=-=slot:%d <<", slot);
    }

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
    cmd_header->cfl = 5; // 20-byte H2D FIS for both ATA and ATAPI
    cmd_header->a = (disk->type == 1); // ATAPI bit
    cmd_header->w = 0; // Read operation
    cmd_header->prdtl = 1; // Single PRD

    // Allocate command table
    cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
    if (!cmd_header->ctba) {
        printf("Error: kmalloc failed\n");
        return -1;
    }
    cmd_header->ctbau = 0; // 32-bit addressing

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)cmd_header->ctba;
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));

    // Setup PRD
    uint32_t sector_size = (disk->type == 1) ? 2048 : 512;
    uint32_t transfer_bytes = (command == AHCI_CMD_IDENTIFY) ? 512 : count * sector_size;
    cmd_tbl->prdt_entry[0].dba = (uint32_t)buffer;
    cmd_tbl->prdt_entry[0].dbau = 0;
    cmd_tbl->prdt_entry[0].dbc = transfer_bytes - 1; // 0-based count
    cmd_tbl->prdt_entry[0].i = 1; // Interrupt on completion

    if (disk->type == 1) {
        // ATAPI (CDROM) command
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
        // ATA (HDD) command
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

    // print_hba_port(port); // Debug port state before command
    port->ci = 1 << slot; // Issue command

    uint32_t timeout = 1000000;
    while ((port->ci & (1 << slot)) && !(port->is & (1 << 30)) /*&& timeout--*/) {
        asm volatile("pause");
    }
    // printf("-----------------------------\n");
    // print_ahci_command_state(port, cmd_header, cmd_tbl, disk->type);
    // printf("\n---------");
    // while(1);
    // printf("\n(/^/)y1 slot:%d ci:%08x cixslot:%08x is:%08x isx30:%08x \n",
    //      slot, port->ci,port->ci & (1 << slot), port->is,!(port->is & (1 << 30) ));

    int result = 0;
    if (timeout == 0) {
        printf("Error: Command timeout, TFD=0x%08x, SERR=0x%08x\n", port->tfd, port->serr);
        print_hba_port(port);
        result = -1;
        // Reset port to recover
        port->sctl |= (1 << 0);
        timeout = 1000000;
        while (port->ssts & (1 << 0) && timeout--) asm volatile("pause");
        port->sctl &= ~(1 << 0);
    } else if (port->is & (1 << 30)) {
        printf("Error: Device fault, TFD=0x%08x, SERR=0x%08x\n", port->tfd, port->serr);
        print_hba_port(port);
        result = -1;
        // Reset port to recover
        port->sctl |= (1 << 0);
        timeout = 1000000;
        while (port->ssts & (1 << 0) && timeout--) asm volatile("pause");
        port->sctl &= ~(1 << 0);
    }

    // printf("<%d>i=0x%08x, slot=0x%08x\n", result, port->ci, 1 << slot);
    kfree((void*)cmd_header->ctba);
    return result;
}

// int ahci_read_sectors2(Disk* disk, uint32_t lba, uint32_t count, void* buffer, int command) {
//     // Log parameters for debugging
//     printf("ahci_read: port=%08x, type=%d, lba=%u, count=%u, buffer=%08x, cmd=%d\n",
//            disk->port, disk->type, lba, count, buffer, command);
//     // - disk->port: HBA_PORT pointer for the device
//     // - disk->type: 0 (HDD, ATA) or 1 (CDROM, ATAPI)
//     // - lba: Logical Block Address (ignored for IDENTIFY)
//     // - count: Number of sectors (1 for IDENTIFY, variable for READ)
//     // - buffer: Destination memory for data
//     // - command: AHCI_CMD_READ or AHCI_CMD_IDENTIFY

//     HBA_PORT* port = disk->port;
//     // Access port from Disk struct
//     // - Assumes disk->port is set by ahci_init()

//     port->is = 0xFFFFFFFF;
//     // Clear interrupt status register
//     // - Writes 1 to clear all interrupt bits (e.g., completion, errors)
//     // - Ensures new interrupts are detected

//     int slot = 0;
//     // Use command slot 0 (AHCI supports 0-31 slots)
//     // - Simplifies logic; assumes slot is free
//     // - TODO: Check port->ci for slot availability

//     HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
//     // Point to command header in command list
//     // - port->clb: Command List Base, set by port_rebase()
//     // - Each header is 32 bytes; slot 0 at offset 0

//     if (disk->type == 1) {
//         // ATAPI (CDROM) command
//         cmd_header->cfl = 3; // 12-byte SCSI command (3 dwords with padding)
//     else{
//         cmd_header->cfl = 5; // 20-byte H2D FIS
//     }


//     cmd_header->a = (disk->type == 1); // ATAPI bit
//     // Set A-bit for ATAPI devices (CDROM)
//     // - 1 for ATAPI (sends SCSI packet), 0 for ATA (HDD)

//     cmd_header->w = 0; // Read operation
//     // Clear W-bit (0 = read, 1 = write)
//     // - Correct for reading data or IDENTIFY response

//     uint32_t sector_size = (disk->type == 1) ? 2048 : 512;
//     // Sector size: 2048 bytes for CDROM, 512 for HDD
//     // - CDROM: ISO9660 uses 2048-byte sectors
//     // - HDD: Standard 512-byte sectors
//     // - IDENTIFY: Always 512 bytes (1 sector)

//     cmd_header->prdtl = 1; // Single PRD
//     // Physical Region Descriptor Table Length
//     // - Use one PRD for simplicity (handles up to 4MB)
//     // - Sufficient for IDENTIFY (512 bytes) and typical reads

//     cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
//     // Allocate Command Table
//     // - Holds Command FIS (CFIS), ATAPI command (acmd), and PRDT
//     if (!cmd_header->ctba) {
//         printf("Error: kmalloc failed\n");
//         return -1;
//     }
//     cmd_header->ctbau = 0; // 32-bit addressing
//     // Upper 32 bits of CTBA (0 since no paging)

//     HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
//     memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));
//     // Clear Command Table to avoid stale data

//     // Setup PRD
//     uint32_t transfer_bytes = (command == AHCI_CMD_IDENTIFY) ? 512 : count * sector_size;
//     cmd_tbl->prdt_entry[0].dba = (uint32_t)buffer;
//     cmd_tbl->prdt_entry[0].dbau = 0;
//     cmd_tbl->prdt_entry[0].dbc = transfer_bytes - 1; // 0-based count
//     cmd_tbl->prdt_entry[0].i = 1; // Interrupt on completion
//     // - dba: Buffer address for data
//     // - dbau: 0 for 32-bit
//     // - dbc: Bytes - 1 (e.g., 512 = 511, 2048 = 2047)
//     // - i: Optional interrupt (set for safety)

//     if (disk->type == 1) {
//         // ATAPI (CDROM) command
//         cmd_header->cfl = 6; // 12-byte SCSI command (6 dwords with padding)
//         // - QEMU expects 12-byte packet; cfl in dwords (12 bytes = 3 dwords, padded)
//         // - Set before PRDT for consistency

//         cmd_tbl->cfis[0] = 0x27; // H2D FIS
//         cmd_tbl->cfis[1] = 0x80; // Command bit
//         cmd_tbl->cfis[2] = 0xA0; // PACKET command
//         // - 0x27: Host-to-Device FIS
//         // - 0x80: C-bit (command, not control)
//         // - 0xA0: ATA PACKET command for ATAPI

//         memset(cmd_tbl->acmd, 0, 16); // Clear SCSI command buffer
//         if (command == AHCI_CMD_IDENTIFY) {
//             // ATAPI IDENTIFY PACKET DEVICE
//             cmd_tbl->acmd[0] = 0xA1; // IDENTIFY PACKET DEVICE
//             cmd_tbl->acmd[1] = 0x00; // No special flags
//             cmd_tbl->acmd[2] = 0x00; // Reserved
//             cmd_tbl->acmd[3] = 0x00; // Reserved
//             cmd_tbl->acmd[4] = 0x01; // Request 1 sector (512 bytes)
//             // - Returns 512-byte identification data
//             // - QEMU ide-cd supports this command
//         } else {
//             // SCSI READ(10) for data read
//             cmd_tbl->acmd[0] = 0x28; // READ(10)
//             cmd_tbl->acmd[2] = (lba >> 24) & 0xFF;
//             cmd_tbl->acmd[3] = (lba >> 16) & 0xFF;
//             cmd_tbl->acmd[4] = (lba >> 8) & 0xFF;
//             cmd_tbl->acmd[5] = lba & 0xFF;
//             cmd_tbl->acmd[7] = (count >> 8) & 0xFF;
//             cmd_tbl->acmd[8] = count & 0xFF;
//             // - 0x28: SCSI READ(10), compatible with QEMU ide-cd
//             // - LBA: 32-bit, big-endian
//             // - Count: 16-bit, big-endian
//         }
//     } else {
//         // ATA (HDD) command
//         cmd_header->cfl = 5; // 20-byte H2D FIS
//         cmd_tbl->cfis[0] = 0x27; // H2D FIS
//         cmd_tbl->cfis[1] = 0x80; // Command bit
//         cmd_tbl->cfis[2] = (command == AHCI_CMD_IDENTIFY) ? 0xEC : 0xC8;
//         // - 0xEC: IDENTIFY DEVICE for identification
//         // - 0xC8: READ DMA for data read
//         if (command == AHCI_CMD_READ) {
//             cmd_tbl->cfis[4] = lba & 0xFF;
//             cmd_tbl->cfis[5] = (lba >> 8) & 0xFF;
//             cmd_tbl->cfis[6] = (lba >> 16) & 0xFF;
//             cmd_tbl->cfis[7] = (lba >> 24) & 0xFF;
//             cmd_tbl->cfis[12] = count & 0xFF;
//             cmd_tbl->cfis[13] = (count >> 8) & 0xFF;
//             // - LBA: 28-bit for READ DMA
//             // - Count: 16-bit
//         } else {
//             // IDENTIFY DEVICE needs no LBA or count
//             cmd_tbl->cfis[4] = 0;
//             cmd_tbl->cfis[5] = 0;
//             cmd_tbl->cfis[6] = 0;
//             cmd_tbl->cfis[7] = 0;
//             cmd_tbl->cfis[12] = 0;
//             cmd_tbl->cfis[13] = 0;
//         }
//     }

//     port->ci = 1 << slot;
//     // Issue command by setting Command Issue bit
//     // - Starts AHCI command processing

//     uint32_t timeout = 1000000; // ~1ms per iteration
//     while ((port->ci & (1 << slot)) && !(port->is & (1 << 30)) && timeout--) {
//         asm volatile("pause");
//     }
//     // Poll for completion or error
//     // - CI bit clears when command completes
//     // - IS.DF (bit 30) indicates Device Fault
//     // - Timeout prevents hangs

//     int result = 0;
//     if (timeout == 0) {
//         printf("Error: Command timeout, TFD=0x%08x\n", port->tfd);
//         result = -1;
//     } else if (port->is & (1 << 30)) {
//         printf("Error: Device fault, TFD=0x%08x\n", port->tfd);
//         result = -1;
//     }
//     // Check for timeout or error
//     // - Logs TFD (Task File Data) for diagnosis
//     // - TFD: Status (bits 7:0), Error (bits 15:8)

//     printf("timeout=%u, ci=%08x, slot=%08x\n", timeout, port->ci, 1 << slot);
//     // Log completion status
//     // - timeout: Remaining iterations
//     // - ci: Command Issue register
//     // - slot: Issued slot mask

//     kfree((void*)cmd_header->ctba);
//     // Free Command Table to avoid memory leak
//     return result;
// }

int ahci_read_sectors(Disk* disk, uint32_t lba, uint32_t count, void* buffer) {
    // printf("ahci_read: port=%08x, type=%d, lba=%u, count=%u, buffer=%08x\n",
    //        disk->port, disk->type, lba, count, buffer);
    HBA_PORT* port = disk->port;
    // HBA_PORT* port = (HBA_PORT*)((uint8_t*)hba_mem + 0x100 + disks[num_disks].port_index * 0x80);
    port->is = 0xFFFFFFFF;

    int slot = 0;
    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
    cmd_header->cfl = 5; // Default FIS length for ATA
    cmd_header->a = (disk->type == 1); // ATAPI bit
    cmd_header->w = 0;
    uint32_t sector_size = (disk->type == 1) ? 2048 : 512; // CDROM: 2048 bytes, HDD: 512
    cmd_header->prdtl = (count + 7) / 8;
    cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
    if (!cmd_header->ctba) {
        printf("Error: kmalloc failed\n");
        return -1;
    }
    cmd_header->ctbau = 0;

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));
    uint8_t* buf_ptr = buffer;
    for (uint32_t i = 0; i < cmd_header->prdtl; i++) {
        uint32_t sectors = (count > 8) ? 8 : count;
        cmd_tbl->prdt_entry[i].dba = (uint32_t)buf_ptr;
        cmd_tbl->prdt_entry[i].dbau = 0;
        cmd_tbl->prdt_entry[i].dbc = sectors * sector_size - 1; // Adjust for sector size
        cmd_tbl->prdt_entry[i].i = 1;
        buf_ptr += sectors * sector_size;
        count -= sectors;
    }

    if (disk->type == 1) {
        // ATAPI packet command for CDROM
        cmd_header->cfl = 3; // 12-byte packet (3 dwords)
        cmd_tbl->cfis[0] = 0x27; // H2D FIS
        cmd_tbl->cfis[1] = 0x80; // Command bit
        cmd_tbl->cfis[2] = 0xA0; // PACKET command for ATAPI
        // ATAPI READ(12) packet
        cmd_tbl->acmd[0] = 0xA8; // READ(12)
        cmd_tbl->acmd[2] = (lba >> 24) & 0xFF; // LBA
        cmd_tbl->acmd[3] = (lba >> 16) & 0xFF;
        cmd_tbl->acmd[4] = (lba >> 8) & 0xFF;
        cmd_tbl->acmd[5] = lba & 0xFF;
        cmd_tbl->acmd[9] = (count >> 8) & 0xFF; // Transfer length
        cmd_tbl->acmd[10] = count & 0xFF;
    } else {
        // ATA command for HDD
        cmd_tbl->cfis[0] = 0x27;
        cmd_tbl->cfis[1] = 0x80;
        cmd_tbl->cfis[2] = 0xC8; // READ DMA
        cmd_tbl->cfis[4] = lba & 0xFF;
        cmd_tbl->cfis[5] = (lba >> 8) & 0xFF;
        cmd_tbl->cfis[6] = (lba >> 16) & 0xFF;
        cmd_tbl->cfis[7] = (lba >> 24) & 0xFF;
        cmd_tbl->cfis[12] = count & 0xFF;
        cmd_tbl->cfis[13] = (count >> 8) & 0xFF;
    }

    port->ci = 1 << slot;
    int32_t timeout = 1000000;
    while ((port->ci & (1 << slot)) && !(port->is & (1 << 30)) && timeout) {
        asm volatile("pause");
    }
    if (timeout <= 0) {
        printf("Error: Read timeout\n");
        kfree((void*)cmd_header->ctba);
        return -1;
    }else{
        // printf("time:%d ci:%08x slot:%08x ", timeout, port->ci, (1<<slot));
    }
    int result = (port->is & (1 << 30)) ? -1 : 0;
    if (result != 0) {
        printf("Error: Read failed, TFD=0x%08x\n", port->tfd);
    }
    kfree((void*)cmd_header->ctba);
    return result;
}

// int ahci_read_sectors(Disk* disk, uint32_t lba, uint32_t count, void* buffer) {
//     HBA_PORT* port = disk->port;
//     port->is = 0xFFFFFFFF;

//     int slot = 0;
//     HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)((uint8_t*)port->clb + slot * 32);
//     cmd_header->cfl = 5;
//     cmd_header->a = (disk->type == 1);
//     cmd_header->w = 0;
//     cmd_header->prdtl = (count + 7) / 8;
//     cmd_header->ctba = (uint32_t)kmalloc(sizeof(HBA_CMD_TBL));
//     cmd_header->ctbau = 0;
//     // printf(" c1md_header->ctba:%08x ", cmd_header->ctba);

//     HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
//     memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));
//     uint8_t* buf_ptr = buffer;
//     for (uint32_t i = 0; i < cmd_header->prdtl; i++) {
//         uint32_t sectors = (count > 8) ? 8 : count;
//         cmd_tbl->prdt_entry[i].dba = (uint32_t)buf_ptr;
//         cmd_tbl->prdt_entry[i].dbau = 0;
//         cmd_tbl->prdt_entry[i].dbc = sectors * 512 - 1;
//         cmd_tbl->prdt_entry[i].i = 1;
//         buf_ptr += sectors * 512;
//         count -= sectors;
//     }

//     cmd_tbl->cfis[0] = 0x27;
//     cmd_tbl->cfis[1] = 0x80;
//     cmd_tbl->cfis[2] = (disk->type == 0) ? 0xC8 : 0x25;
//     cmd_tbl->cfis[4] = lba & 0xFF;
//     cmd_tbl->cfis[5] = (lba >> 8) & 0xFF;
//     cmd_tbl->cfis[6] = (lba >> 16) & 0xFF;
//     cmd_tbl->cfis[7] = (lba >> 24) & 0xFF;
//     cmd_tbl->cfis[12] = count & 0xFF;
//     cmd_tbl->cfis[13] = (count >> 8) & 0xFF;

//     port->ci = 1 << slot;
//     while ((port->ci & (1 << slot)) && !(port->is & (1 << 30))) {
//         asm volatile("pause");
//     }
//     int result = (port->is & (1 << 30)) ? -1 : 0;
//     // printf(" c2md_header->ctba:%08x ", cmd_header->ctba);
//     kfree((void*)cmd_header->ctba);
//     return result;
// }

int ahci_write_sectors(Disk* disk, uint32_t lba, uint32_t count, const void* buffer) {
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
    for (uint32_t i = 0; i < cmd_header->prdtl; i++) {
        uint32_t sectors = (count > 8) ? 8 : count;
        cmd_tbl->prdt_entry[i].dba = (uint32_t)buf_ptr;
        cmd_tbl->prdt_entry[i].dbau = 0;
        cmd_tbl->prdt_entry[i].dbc = sectors * 512 - 1;
        cmd_tbl->prdt_entry[i].i = 1;
        buf_ptr += sectors * 512;
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