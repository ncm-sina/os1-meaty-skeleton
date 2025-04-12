#ifndef _KERNEL_PROCESS_H
#define _KERNEL_PROCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/multiboot.h>

#define MAX_PROCESSES 256

typedef enum {
    PROCESS_FREE = 0,    // Slot is unused
    PROCESS_RUNNING = 1, // Currently executing
    PROCESS_READY = 2,   // Ready to run
    PROCESS_BLOCKED = 3, // Waiting (e.g., I/O, semaphore)
    PROCESS_TERMINATED = 4 // Done, awaiting cleanup
} ProcessStateEnum;

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t cs, ds, es, fs, gs, ss;
    uint32_t eflags;
} Registers;

typedef struct {
    uint32_t pid;
    uint32_t* page_dir;
    ProcessStateEnum state;
    uint32_t priority;
    uint32_t time_slice;
    uint32_t kernel_esp;
    Registers regs;
} Process;

typedef struct {
    Process process_table[MAX_PROCESSES];
    uint32_t next_pid;
    uint32_t current_pid;
} ProcessState;

// New struct for binary info
typedef struct {
    uint32_t start_addr; // Physical address of binary start
    uint32_t end_addr;   // Physical address of binary end
} binary_info_t;

extern ProcessState process_state;

void show_processes(int start, int count);
void init_processes();
uint32_t create_process(binary_info_t* bin);
void load_multiboot_mod(multiboot_module_t* mod);
void load_multiboot_mods(multiboot_info_t* mbi);
void switch_to_process(uint32_t pid);
void schedule();

#endif