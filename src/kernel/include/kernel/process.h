#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_PROCESSES 256
#define USER_STACK_TOP 0xBFFFFFFF // User stack top
#define USER_STACK_SIZE 0x10000   // 64KB user stack
#define KERNEL_STACK_SIZE 0x1000  // 4KB kernel stack
#define USER_CODE_ADDR 0x1000     // Default code address for ELF
#define USER_CODE_SELECTOR 0x18   // Example GDT user code selector (ring 3)
#define USER_DATA_SELECTOR 0x20   // Example GDT user data selector (ring 3)

#include <kernel/multiboot.h>


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
} __attribute__((packed)) Registers;

typedef struct {
    uint32_t pid;
    uint32_t* page_dir;
    ProcessStateEnum state;
    uint32_t priority;
    uint32_t time_slice;
    uint32_t kernel_esp;
    Registers regs;
} __attribute__((packed)) Process;

typedef struct {
    Process process_table[MAX_PROCESSES];
    uint32_t next_pid;
    uint32_t current_pid;
} __attribute__((packed)) ProcessState;

// New struct for binary info
typedef struct {
    uint32_t start_addr; // Physical address of binary start
    uint32_t end_addr;   // Physical address of binary end
} __attribute__((packed)) binary_info_t;

typedef struct {
    uint32_t text_start;
    uint32_t text_end;
    uint32_t data_start;
    uint32_t data_end;
} __attribute__((packed)) ModuleHeader_t;

// Function pointer type for main_func_ptr
typedef void (*func_ptr2_t)(Registers *r);
// typedef void (*func_ptr2_t)(void);


extern func_ptr2_t schedule_asm;
extern func_ptr2_t switch_to_process_asm;

extern ProcessState process_state;

// void show_processes(int start, int count);
int init_process();
int execute(const char *absPath, ...);
void switch_to_process(uint32_t pid);

multiboot_module_t* get_multiboot_mod_by_name(multiboot_info_t* mbi, const char* name);
void load_multiboot_mod(multiboot_module_t* mod);
int load_multiboot_mods(multiboot_info_t* mbi);
void schedule();

#endif