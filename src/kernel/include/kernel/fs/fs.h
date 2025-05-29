#ifndef FS_H
#define FS_H

#include <stdint.h>

/* Load a file from the filesystem into memory.
 * absPath: Absolute path to the file (e.g., "/sbin/init.elf").
 * data: For flat binaries, points to a binary_info_t*; for ELF, points to a uint8_t** for raw data.
 * size: If non-NULL, set to the file's size (used for ELF).
 * Returns: 0 on success, negative on error.
 */
int loadFile(const char *absPath, void *data, uint32_t *size);

#endif