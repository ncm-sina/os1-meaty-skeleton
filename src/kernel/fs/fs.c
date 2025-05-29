#include <kernel/fs/fs.h>
#include <memory.h>
#include <string.h>
#include <kernel/process.h>

/* Simulated ramdisk structure (replace with your actual filesystem) */
typedef struct {
    const char *path;
    uint8_t *data;
    uint32_t size;
} RamdiskEntry;

static RamdiskEntry ramdisk[] = {
    {"/sbin/init.elf", NULL, 0},  // Placeholder: actual data set at runtime
    {"/sbin/shell.elf", NULL, 0}, // Placeholder
    {"/sbin/myprog.bin", NULL, 0},// Placeholder for flat binary
    {NULL, NULL, 0}
};

/* Simulate reading from ramdisk (replace with real filesystem read) */
static int ramdisk_read(const char *path, uint8_t **data, uint32_t *size) {
    for (int i = 0; ramdisk[i].path; i++) {
        if (strcmp(path, ramdisk[i].path) == 0) {
            *data = ramdisk[i].data;
            *size = ramdisk[i].size;
            return 0;
        }
    }
    return -1; // File not found
}

int loadFile(const char *absPath, void *data, uint32_t *size) {
    if (!absPath || !data) return -1;

    uint8_t *file_data;
    uint32_t file_size;
    if (ramdisk_read(absPath, &file_data, &file_size) != 0) {
        return -2; // File not found
    }

    if (size) {
        /* ELF mode: allocate memory and copy file contents */
        *(uint8_t **)data = kmalloc(file_size);
        if (!*(uint8_t **)data) {
            return -3; // Out of memory
        }
        memcpy(*(uint8_t **)data, file_data, file_size);
        *size = file_size;
    } else {
        /* Flat binary mode: allocate physical pages and map */
        binary_info_t *bin_info = (binary_info_t *)data;
        uint32_t phys_addr = allocate_physical_pages(file_size);
        if (!phys_addr) {
            return -3; // Out of memory
        }

        /* Copy file data to physical memory */
        memcpy((void *)phys_addr, file_data, file_size);

        /* Update binary_info_t */
        bin_info->start_addr = phys_addr;
        bin_info->end_addr = phys_addr + file_size;
    }

    return 0;
}