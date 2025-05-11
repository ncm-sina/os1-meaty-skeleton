#ifndef PRINT_XX_H
#define PRINT_XX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include "ahci.h"
#include "iso9660.h"
#include "fat32.h"

// Print key fields of HBA_PORT
void print_hba_port(HBA_PORT* port);

// Print key fields of HBA_MEM
void print_hba_mem(HBA_MEM* mem);

// Print key fields of HBA_CMD_TBL
void print_hba_cmd_tbl(HBA_CMD_TBL* tbl);

// Print key fields of VolumeDescriptor
void print_volume_descriptor(VolumeDescriptor* vd);

// Print key fields of DirectoryRecord
void print_directory_record(DirectoryRecord* dr);

// Print key fields of DirEntry
void print_dir_entry(DirEntry* de);

// Print key fields of BootSector
void print_boot_sector(BootSector* bs);

void print_ahci_command_state(HBA_PORT* port, HBA_CMD_HEADER* cmd_header, HBA_CMD_TBL* cmd_tbl, int is_atapi);
#endif