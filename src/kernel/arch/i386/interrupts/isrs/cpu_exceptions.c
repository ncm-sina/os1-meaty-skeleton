#include <kernel/arch/i386/isrs/cpu_exceptions.h>
#include <kernel/arch/i386/paging.h>
#include <kernel/fpu.h>

#include <stdio.h>

void isr_divide_error(void) {
    printf("Divide by zero error\n");
    asm volatile("cli; hlt");
}

void isr_debug(void) {
    printf("Debug exception\n");
}

void isr_nmi(void) {
    printf("Non-maskable interrupt\n");
}

void isr_breakpoint(void) {
    printf("Breakpoint\n");
}

void isr_overflow(void) {
    printf("Overflow\n");
}

void isr_bound_range(void) {
    printf("Bound range exceeded\n");
}

void isr_invalid_opcode(void) {
    printf("Invalid opcode\n");
    asm volatile("cli; hlt");
}

void isr_device_not_available(void) {
    if (!is_fpu_enabled()) {
        printf("FPU not enabled, enabling now...\n");
        enable_fpu(); // Enable FPU on demand (for simplicity now)
    } else {
        printf("Unexpected FPU exception (ISR 7)\n");
        // In a multitasking OS, this would switch FPU context
    }
}

void isr_double_fault(uint32_t error_code) {
    printf("Double fault, error: %08X\n", error_code);
    asm volatile("cli; hlt");
}

void isr_invalid_tss(uint32_t error_code) {
    printf("Invalid TSS, error: %08X\n", error_code);
    asm volatile("cli; hlt");
}

void isr_segment_not_present(uint32_t error_code) {
    printf("Segment not present, error: %08X\n", error_code);
    asm volatile("cli; hlt");
}

void isr_stack_segment_fault(uint32_t error_code) {
    printf("Stack segment fault, error: %08X\n", error_code);
    asm volatile("cli; hlt");
}

void isr_general_protection(uint32_t error_code) {
    printf("General protection fault, error: %08X\n", error_code);
    asm volatile("cli; hlt");
}

void isr_page_fault(uint32_t error_code) {
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r" (fault_addr));

    if (!(error_code & 0x1)) { // Page not present
        uint32_t* current_pd = get_current_page_directory();
        int result = assign_page_table(current_pd, (void*)(fault_addr & ~0x3FFFFF), 
                                      (error_code & 0x4) ? PAGE_USER : 0);
        if (result == 0) {
            return; // Successfully assigned a page table, resume execution
        }
        printf("Page Fault: No free page tables for %08X err: %d \n", fault_addr, result);
    } else {
        printf("Page Fault: Protection violation at %08X\n", fault_addr);
    }

    printf("Error code: %08X\n", error_code);
    asm volatile("cli; hlt");
}   