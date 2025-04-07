#ifndef ISRS_CPU_EXCEPTIONS_H
#define ISRS_CPU_EXCEPTIONS_H

#include <stdint.h>

void isr_divide_error(void);
void isr_debug(void);
void isr_nmi(void);
void isr_breakpoint(void);
void isr_overflow(void);
void isr_bound_range(void);
void isr_invalid_opcode(void);
void isr_device_not_available(void);
void isr_double_fault(uint32_t error_code);
void isr_invalid_tss(uint32_t error_code);
void isr_segment_not_present(uint32_t error_code);
void isr_stack_segment_fault(uint32_t error_code);
void isr_general_protection(uint32_t error_code);
void isr_page_fault(uint32_t error_code);

#endif