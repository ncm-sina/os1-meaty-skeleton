#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/arch/i386/isrs/timer.h>
#include <kernel/process.h>

volatile int isr_timer_tick_flag = 0;
static void (*timer_handler)(void) = NULL; // Handler remains static, only accessed via register function

// Timer ISR: Sets tick flag and calls registered handler
static inline void scheduler(){
    Process* current = &process_state.process_table[process_state.current_pid];
    if (current->state != 1) return; // Not running

    // Save full state from interrupt stack frame
    asm volatile(
        "pusha\n" // Saves eax, ebx, ecx, edx, esi, edi, ebp, esp (before pusha)
        "mov %%ds, %%eax\n"
        "push %%eax\n" // Save DS
        :
        : 
        : "memory"
    );

    // Pop registers into struct (adjust stack offsets based on your ISR wrapper)
    uint32_t* stack_ptr;
    asm volatile("mov %%esp, %0" : "=r" (stack_ptr));
    current->regs.eax = stack_ptr[8]; // After pusha
    current->regs.ebx = stack_ptr[7];
    current->regs.ecx = stack_ptr[6];
    current->regs.edx = stack_ptr[5];
    current->regs.esi = stack_ptr[4];
    current->regs.edi = stack_ptr[3];
    current->regs.ebp = stack_ptr[1];
    current->regs.esp = stack_ptr[0] + 36; // Adjust for pushed state
    current->regs.ds = stack_ptr[9];
    current->regs.es = current->regs.ds;
    current->regs.fs = current->regs.ds;
    current->regs.gs = current->regs.ds;

    // Get eip, cs, eflags, esp, ss from interrupt stack frame
    current->regs.eip = stack_ptr[12];
    current->regs.cs = stack_ptr[13];
    current->regs.eflags = stack_ptr[14];
    current->regs.ss = stack_ptr[16];
    if (current->regs.cs & 0x3) { // User mode
        current->regs.esp = stack_ptr[15];
    }

    asm volatile("pop %%eax\n popa" : : : "eax"); // Clean up stack

    schedule(); // Switch to next process

}


void isr_timer(void) {
    isr_timer_tick_flag = 1;
    if (timer_handler) {
        timer_handler();
    }

    // scheduler();
    

}

// Register a handler function to process timer ticks
void isr_timer_register_handler(void (*handler)(void)) {
    timer_handler = handler;
}