#ifndef _KERNEL_MCPUID_H
#define _KERNEL_MCPUID_H

#include <kernel/paging.h>

void b_test_fpu();
void b_test_floating_point(void);
void b_test_isr_driver();
void b_test_idt();
void b_test_mbi(multiboot_info_t* mbi);
void b_test_a20();
void b_test_multiboot_header();
void b_test_mconio();

void b_test_mconio_scroll();
void b_test_cupid();

#endif // MCPUID_H