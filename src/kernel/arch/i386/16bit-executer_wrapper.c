#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/arch/i386/16bit-executer_wrapper.h>
// #include <kernel/process.h>

#ifndef MAX_MOD_SIZE
#define MAX_MOD_SIZE 0x1000
#endif


uint32_t call_rm_program(rm_binary_info_t bin_info) {
    uint8_t* src = (uint8_t*)bin_info.text_start_addr;
    uint8_t* dst = (uint8_t*)0x2000;
    for(uint32_t i = 0; i < bin_info.text_size && i<MAX_MOD_SIZE; i++){
        dst[i] = src[i];
    }


    // uint32_t *mentry = 0x1000;
    uint32_t (*exec_rm_program)(rm_binary_info_t) = (uint32_t (*)(rm_binary_info_t))(0x1000); /* exec_rm_program_ptr */
    return (exec_rm_program)(bin_info);
}