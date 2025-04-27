#include "16bit-executer.h"
#include <stdint.h>
#include <stdio.h>


// External assembly functions
extern void _init_mode_switcher();
extern void _load_rm_program(rm_binary_info_t bin_info, func_ptr_t *main_func_ptr);
extern uint32_t _run_rm_program(func_ptr_t *main_func_ptr);

// extern void enter_real_mode(void);
// extern void return_to_protected_mode(void);

// uint32_t run_rm_program(rm_binary_info_t bin_info){
//     // Declare function pointer
//     func_ptr_t main_func_ptr = NULL;
//     // _init_mode_switcher();
//     _load_rm_program(bin_info, &main_func_ptr);
//     // Check if main_func_ptr was set
//     if (main_func_ptr != NULL) {
//         // Call the function at 0x02000
//         return _run_rm_program(&main_func_ptr);
//     }
//     return -1;
// }
