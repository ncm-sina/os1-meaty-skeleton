#include "print_xx.h"

// Print key fields of HBA_PORT
void print_hba_port(HBA_PORT* port) {
    printf("HBA_PORT: ssts=0x%08x, sig=0x%08x, tfd=0x%08x, ci=0x%08x\n",
           port->ssts, port->sig, port->tfd, port->ci);
}

// Print key fields of HBA_MEM
void print_hba_mem(HBA_MEM* mem) {
    printf("HBA_MEM: cap=0x%08x, ghc=0x%08x, pi=0x%08x, vs=0x%08x\n",
           mem->cap, mem->ghc, mem->pi, mem->vs);
}

// Print key fields of HBA_CMD_TBL
void print_hba_cmd_tbl(HBA_CMD_TBL* tbl) {
    printf("HBA_CMD_TBL: cfis[0]=0x%02x, acmd[0]=0x%02x, prdt[0].dba=0x%08x, prdt[0].dbc=%u\n",
           tbl->cfis[0], tbl->acmd[0], tbl->prdt_entry[0].dba, tbl->prdt_entry[0].dbc);
}

// Print key fields of VolumeDescriptor
void print_volume_descriptor(VolumeDescriptor* vd) {
    char id[6] = {0};
    char vol_id[33] = {0};
    strncpy(id, vd->identifier, 5);
    strncpy(vol_id, vd->volume_id, 32);
    printf("VolumeDescriptor: type=%u, id=%s, vol_id=%s, block_size=%u, vol_size=%u\n",
           vd->type, id, vol_id, vd->logical_block_size, vd->volume_space_size);
}

// Print key fields of DirectoryRecord
void print_directory_record(DirectoryRecord* dr) {
    char name[32] = {0};
    strncpy(name, dr->name, dr->name_len < 31 ? dr->name_len : 31);
    printf("DirectoryRecord: extent_lba=%u, size=%u, name_len=%u, name=%s\n",
           dr->extent_lba, dr->size, dr->name_len, name);
}

// Print key fields of ISO9660_DirEntry
void print_dir_entry_iso9660(ISO9660_DirEntry* de) {
    printf("ISO9660_DirEntry: name=%s, extent_lba=%u, size=%u, name_len=%u\n",
           de->name ? de->name : "(null)", de->record.extent_lba, de->record.size, de->record.name_len);
}

// Print key fields of DirEntry (FAT32)
void print_dir_entry_fat32(DirEntry* de) {
    char name[12] = {0};
    strncpy(name, de->name, 11);
    printf("DirEntry(FAT32): name=%s, attr=0x%02x, cluster=%u, size=%u\n",
           name, de->attr, (de->first_cluster_high << 16) | de->first_cluster_low, de->file_size);
}

// Print key fields of BootSector
void print_boot_sector(BootSector* bs) {
    char oem[9] = {0};
    char fs_type[9] = {0};
    strncpy(oem, bs->oem_name, 8);
    strncpy(fs_type, bs->fs_type, 8);
    printf("BootSector: oem=%s, bytes_per_sector=%u, sectors_per_cluster=%u, root_cluster=%u, fs_type=%s\n",
           oem, bs->bytes_per_sector, bs->sectors_per_cluster, bs->root_cluster, fs_type);
}

// Print AHCI command state affecting command issuance
void print_ahci_command_state(HBA_PORT* port, HBA_CMD_HEADER* cmd_header, HBA_CMD_TBL* cmd_tbl, int is_atapi) {
    printf("AHCI Command State:\n");
    printf("p:clb=%08x,fb=%08x,cmd=%08x,ssts=%08x,serr=%08x,tfd=%08x,sig=%08x,is=%08x,ie=%08x\n",
           port->clb, port->fb, port->cmd, port->ssts, port->serr, port->tfd, port->sig, port->is, port->ie);
    printf("CMD_HEADER:cfl=%u,a=%u,w=%u,prdtl=%u,ctba=%08x\n",
           cmd_header->cfl, cmd_header->a, cmd_header->w, cmd_header->prdtl, cmd_header->ctba);
    printf("CMD_TBL: cfis=[%02x %02x %02x], prdt[0].dba=%08x, prdt[0].dbc=%u, prdt[0].i=%u\n",
           cmd_tbl->cfis[0], cmd_tbl->cfis[1], cmd_tbl->cfis[2], cmd_tbl->prdt_entry[0].dba, cmd_tbl->prdt_entry[0].dbc, cmd_tbl->prdt_entry[0].i);
    if (is_atapi) {
        printf("  ATAPI acmd=[%02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
               cmd_tbl->acmd[0], cmd_tbl->acmd[1], cmd_tbl->acmd[2], cmd_tbl->acmd[3],
               cmd_tbl->acmd[4], cmd_tbl->acmd[5], cmd_tbl->acmd[6], cmd_tbl->acmd[7], cmd_tbl->acmd[8]);
    }
}