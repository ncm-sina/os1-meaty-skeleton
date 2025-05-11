#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// First, let's do some basic checks to make sure we are using our x86-elf cross-compiler correctly
#if defined(__linux__)
	#error "This code must be compiled with a cross-compiler"
#elif !defined(__i386__)
	#error "This code must be compiled with an x86-elf compiler"
#endif

#include <stdio.h>
#include <string.h>
#include <kernel/arch/i386/idt.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/vga.h>
#include <kernel/mport.h>

#include "./utils/ahci.h"
#include "./utils/fat32.h"
#include "./utils/iso9660.h"
#include "./utils/disk.h"
#include "./utils/memory.h"
#include "./utils/print_xx.h"

#include <kernel/multiboot.h>


extern void* _os_installer_end;

static inline void halt(void) {
    asm volatile ("cli; hlt");  // Halt CPU safely
}

// Static function to hide the cursor using port I/O
static void _hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
}

void print_mmap_addresses(multiboot_info_t* mbi){
        
    if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
        printf("\n info mem map not available\n");
    } else {
        multiboot_memory_map_t* mmap = (void*)mbi->mmap_addr;
        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
        uint32_t max_addr = 0;
        printf("\n mbi address: %#08X", mmap);
        printf("\n #   size\t addr\tend\tlength\ttype\n");
        uint16_t i=0;
        while ((uint32_t)mmap < mmap_end) {
            printf(" %d: %#08x %#08X %#08X %#08X %#08x\n", ++i,
                mmap->size,
                mmap->addr,
                mmap->addr+mmap->len,
                mmap->len,
                mmap->type
                // ((uint32_t)mmap->addr)+((uint32_t) mmap->len),
            );
               mmap = (void*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
        }
}

void print_module_info(multiboot_info_t* mbi) {
    if (!(mbi->flags & MULTIBOOT_INFO_MODS)) {
        printf("\nModule information not available\n");
        return;
    }

    if (mbi->mods_count == 0) {
        printf("\nNo modules loaded\n");
        return;
    }

    multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
    printf("\n #\t   Start\t   End\t\t   Cmdline\n");
    
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        // Get module fields
        uint32_t start = mods[i].mod_start;
        uint32_t end = mods[i].mod_end;
        char* cmdline = (char*)mods[i].cmdline;

        // Print module info (handle null or invalid cmdline)
        printf(" %u:\t%#08x\t%#08x\t%s\n", i, start, end, 
               (cmdline && mods[i].cmdline != 0) ? cmdline : "(none)");
    }
}


void print_multiboot_info(multiboot_info_t *mbi) {
    printf("Multiboot1 Info Structure:\n");
    printf("  Flags: 0x%08X\n", mbi->flags);

    // Memory info (flags bit 0)
    if (mbi->flags & (1 << 0)) {
        printf("  Memory Lower: %u KB\n", mbi->mem_lower);
        printf("  Memory Upper: %u KB\n", mbi->mem_upper);
    } else {
        printf("  Memory Info: Not available (flags bit 0 unset)\n");
    }

    // Boot device (flags bit 1)
    if (mbi->flags & (1 << 1)) {
        printf("  Boot Device: 0x%08X\n", mbi->boot_device);
    } else {
        printf("  Boot Device: Not available (flags bit 1 unset)\n");
    }

    // Command line (flags bit 2)
    if (mbi->flags & (1 << 2)) {
        printf("  Command Line: 0x%08X (%s)\n", mbi->cmdline,
               mbi->cmdline ? (char *)(uintptr_t)mbi->cmdline : "Empty");
    } else {
        printf("  Command Line: Not available (flags bit 2 unset)\n");
    }

    // Modules (flags bit 3)
    if (mbi->flags & (1 << 3)) {
        printf("  Modules Count: %u\n", mbi->mods_count);
        printf("  Modules Address: 0x%08X\n", mbi->mods_addr);
    } else {
        printf("  Modules: Not available (flags bit 3 unset)\n");
    }

    // Symbol table (flags bit 4 or 5)
    if (mbi->flags & (1 << 4)) {
        printf("  Symbol Table (a.out): 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
               mbi->syms[0], mbi->syms[1], mbi->syms[2], mbi->syms[3]);
    } else if (mbi->flags & (1 << 5)) {
        printf("  Symbol Table (ELF): 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
               mbi->syms[0], mbi->syms[1], mbi->syms[2], mbi->syms[3]);
    } else {
        printf("  Symbol Table: Not available (flags bits 4 and 5 unset)\n");
    }

    // Memory map (flags bit 6)
    if (mbi->flags & (1 << 6)) {
        printf("  Memory Map Length: %u\n", mbi->mmap_length);
        printf("  Memory Map Address: 0x%08X\n", mbi->mmap_addr);
    } else {
        printf("  Memory Map: Not available (flags bit 6 unset)\n");
    }

    // Drives (flags bit 7)
    if (mbi->flags & (1 << 7)) {
        printf("  Drives Length: %u\n", mbi->drives_length);
        printf("  Drives Address: 0x%08X\n", mbi->drives_addr);
    } else {
        printf("  Drives: Not available (flags bit 7 unset)\n");
    }

    // Config table (flags bit 8)
    if (mbi->flags & (1 << 8)) {
        printf("  Config Table: 0x%08X\n", mbi->config_table);
    } else {
        printf("  Config Table: Not available (flags bit 8 unset)\n");
    }

    // // Boot loader name (flags bit 9)
    // if (mbi->flags & (1 << 9)) {
    //     printf("  Boot Loader Name: 0x%08X (%s)\n", mbi->boot_loader_name,
    //            mbi->boot_loader_name ? (char *)(uintptr_t)mbi->boot_loader_name : "Empty");
    // } else {
    //     printf("  Boot Loader Name: Not available (flags bit 9 unset)\n");
    // }

    // // APM table (flags bit 10)
    // if (mbi->flags & (1 << 10)) {
    //     printf("  APM Table: 0x%08X\n", mbi->apm_table);
    // } else {
    //     printf("  APM Table: Not available (flags bit 10 unset)\n");
    // }

    // // VBE info (flags bit 11)
    // if (mbi->flags & (1 << 11)) {
    //     printf("  VBE Control Info: 0x%08X\n", mbi->vbe_control_info);
    //     printf("  VBE Mode Info: 0x%08X\n", mbi->vbe_mode_info);
    //     printf("  VBE Mode: 0x%04X\n", mbi->vbe_mode);
    //     printf("  VBE Interface Segment: 0x%04X\n", mbi->vbe_interface_seg);
    //     printf("  VBE Interface Offset: 0x%04X\n", mbi->vbe_interface_off);
    //     printf("  VBE Interface Length: %u\n", mbi->vbe_interface_len);
    // } else {
    //     printf("  VBE Info: Not available (flags bit 11 unset)\n");
    // }

}



static int copy_files(const char* src_base, const char* dst_base);

void installer_main(uint32_t magic, multiboot_info_t* multiboot_info) {
    (void)multiboot_info;
    // print_multiboot_info(multiboot_info);
    // print_mmap_addresses(multiboot_info);
    // print_module_info(multiboot_info);
        
    // _hide_cursor();
    idt_init();           /* Set up IDT for interrupts */  
    init_heap(&_os_installer_end); // os_installer_end not used for now 
    timer_drv.init();
    keyboard_drv.init();    
    if (magic != 0x1BADB002) {
        printf("Error: Invalid multiboot magic\n");
        return;
    }

    ahci_init();
    Disk disks[32];

    int num_disks = ahci_get_disks(disks, 32);
    Disk* cdrom = NULL;
    Disk* hdds[32];
    int hdd_count = 0;
// while(1);
    printf("Detected %d device(s):\n", num_disks);
    for (int i = 0; i < num_disks; i++) {
        if (disks[i].type == 0) {
            printf("%d. Hard Drive (%u MB)\n", hdd_count + 1, disks[i].total_sectors / 2048);
            hdds[hdd_count++] = &disks[i];
        } else if (disks[i].type == 1) {
            printf("-- CDROM (%u MB)\n", disks[i].total_sectors / 512);
            cdrom = &disks[i];
        }
    }

    if (hdd_count == 0) {
        printf("Error: No hard drives detected\n");
        return;
    }
    if (!cdrom) {
        printf("Error: No CDROM detected - using second hard drive as cdrom \n");
        cdrom = &disks[1];
        // return;
    }

    printf("\nSelect a hard drive to install on (1-%d): ", 1/*hdd_count*/);
    int choice=10000; //too big
    kscanf("%d", &choice);
    while (choice < 1 || choice > 1/*hdd_count*/) {
        // printf("ch: %d ", choice);
        printf("\nError: Invalid choice\n try agian: ");
        kscanf("%d", &choice);
    }

    vga_clear();

    Disk* target_disk = hdds[choice - 1];
    printf("Warning: This will erase all data on the selected drive. Proceed? (y/n): ");
    char confirm;
    kscanf("%c", &confirm);
    if (confirm != 'y' && confirm != 'Y') {
        printf("Installation cancelled\n confirm:%c", confirm);
        return;
    }
    printf(" 1 ");

    printf("Creating partition...\n");
    if (disk_create_partition(target_disk, 2048, target_disk->total_sectors - 1) != 0) {
        printf("Error: Failed to create partition\n");
        while(1);
        return;
    }
    printf(" 2 ");
while(1);
    printf("Formatting partition...\n");
    if (fat32_format(target_disk, 1, target_disk->total_sectors - 1) != 0) {
        printf("Error: Failed to format partition\n");
        while(1);
        return;
    }
    printf(" 3 ");

    printf("Mounting filesystems...\n");
    if (fat32_mount(target_disk, 1) != 0) {
        printf("Error: Failed to mount FAT32 filesystem\n");
        while(1);
        return;
    }
    printf(" 4 ");

    if (iso9660_mount(cdrom) != 0) {
        printf("Error: Failed to mount ISO9660 filesystem\n");
        fat32_unmount();
        while(1);
        return;
    }
    printf(" 5 ");

    printf("Copying files from CD to hard drive...\n");
    if (copy_files("/isoroot", "/") != 0) {
        printf("Error: Failed to copy files\n");
        fat32_unmount();
        iso9660_unmount();
        while(1);
        return;
    }

    printf("Installing GRUB...\n");
    if (disk_install_grub(target_disk, 1) != 0) {
        printf("Error: Failed to install GRUB\n");
        fat32_unmount();
        iso9660_unmount();
        while(1);
        return;
    }

    fat32_unmount();
    iso9660_unmount();
    printf("Installation complete. Reboot to boot from the hard drive.\n");
    while(1);
}

static int copy_files(const char* src_base, const char* dst_base) {
    DirEntry entries[128];
    uint32_t count = 128;
    if (iso9660_list_dir(src_base, entries, &count) != 0) {
        printf("Error: Failed to list directory %s\n", src_base);
        return -1;
    }

    for (uint32_t i = 0; i < count; i++) {
        char src_path[256], dst_path[256];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_base, entries[i].name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_base, entries[i].name);

        if (entries[i].attr & 0x10) { // Directory
            if (strcmp(entries[i].name, ".") == 0 || strcmp(entries[i].name, "..") == 0) continue;
            if (fat32_create_dir(dst_path) != 0) {
                printf("Error: Failed to create directory %s\n", dst_path);
                return -1;
            }
            if (copy_files(src_path, dst_path) != 0) return -1;
        } else { // File
            uint8_t* buffer = kmalloc(entries[i].file_size);
            if (!buffer) {
                printf("Error: Memory allocation failed\n");
                return -1;
            }
            uint32_t size = entries[i].file_size;
            if (iso9660_read_file(src_path, buffer, &size) != 0) {
                printf("Error: Failed to read file %s\n", src_path);
                kfree(buffer);
                return -1;
            }
            if (fat32_create_file(dst_path, buffer, size) != 0) {
                printf("Error: Failed to create file %s\n", dst_path);
                kfree(buffer);
                return -1;
            }
            kfree(buffer);
        }
    }
    return 0;
}