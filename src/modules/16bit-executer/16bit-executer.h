#ifndef _16BIT_EXECUTER_H
#define _16BIT_EXECUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// New struct for binary info
typedef struct {
    uint32_t entry_addr;
    uint32_t text_start_addr; // virtual address of binary start
    uint32_t text_size; // virtual address of binary start
    uint32_t data_start_addr; // virtual address of binary start
    uint32_t data_size; // virtual address of binary start
} rm_binary_info_t;

// Function pointer type for main_func_ptr
typedef void (*func_ptr_t)(void);

#endif