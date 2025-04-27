#ifndef _16BIT_EXECUTER_WRAPPER_H
#define _16BIT_EXECUTER_WRAPPER_H

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

uint32_t call_rm_program(rm_binary_info_t bin_info);

#endif