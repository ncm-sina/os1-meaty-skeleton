#include <kernel/process.h>
#include <kernel/arch/i386/paging.h>
#include <stdio.h>
#include <string.h>
#include <kernel/drivers/vbe.h>
#include <kernel/drivers/serial.h>


#define MAX_MOD_SIZE 0xFFFFF

extern uint32_t pagedir[1024];
extern vbe_info_block_t* vbe_info;
extern vbe_mode_info_t* mode_info;



ProcessState process_state = {
    .process_table = {{0}},
    .next_pid = 1,
    .current_pid = 0
};

// Map ProcessStateEnum to string
static const char* process_state_to_string(ProcessStateEnum state) {
    switch (state) {
        case PROCESS_FREE:      return "FREE";
        case PROCESS_RUNNING:   return "RUNNING";
        case PROCESS_READY:     return "READY";
        case PROCESS_BLOCKED:   return "BLOCKED";
        case PROCESS_TERMINATED: return "TERMINATED";
        default:                return "UNKNOWN";
    }
}

void show_processes(int start, int count) {
    // Validate inputs
    if (start < 0 || start >= MAX_PROCESSES || count <= 0) {
        serial_printf("Invalid parameters: start=%d, count=%d\n", start, count);
        return;
    }

    // Adjust count to stay within bounds
    if (start + count > MAX_PROCESSES) {
        count = MAX_PROCESSES - start;
    }

    // Print header
    serial_printf("\n PID\t PageDir \tStatus\n");
    serial_printf("----\t ------- \t--------\n");

    // Print process info
    int printedProcesses=0;
    for (int i = start; i<MAX_PROCESSES; i++) {
        Process* proc = &process_state.process_table[i];
        if(!proc || proc->pid == 0) continue;
        printedProcesses++;
        serial_printf("%3d\t %08X\t %s\n",
            proc->pid,
            proc->page_dir ? (uint32_t)proc->page_dir : 0,
            process_state_to_string(proc->state));

        if(printedProcesses>=count) break;
    }
}

int init_process() {
    serial_printf(" init processes ");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_state.process_table[i].pid = 0;
        process_state.process_table[i].page_dir = NULL;
        process_state.process_table[i].state = 0;
        process_state.process_table[i].priority = 0;
        process_state.process_table[i].time_slice = 10;
        process_state.process_table[i].kernel_esp = 0;
        // process_state.process_table[i].regs = (Registers){0};
        process_state.process_table[i].regs.eax = 0;
        process_state.process_table[i].regs.ebx = 0;
        process_state.process_table[i].regs.ecx = 0;
        process_state.process_table[i].regs.edx = 0;
        process_state.process_table[i].regs.esi = 0;
        process_state.process_table[i].regs.edi = 0;
        process_state.process_table[i].regs.ebp = 0;
        process_state.process_table[i].regs.esp = 0;
        process_state.process_table[i].regs.eip = 0;
        process_state.process_table[i].regs.cs = 0;
        process_state.process_table[i].regs.ds = 0;
        process_state.process_table[i].regs.es = 0;
        process_state.process_table[i].regs.fs = 0;
        process_state.process_table[i].regs.gs = 0;
        process_state.process_table[i].regs.ss = 0;
        process_state.process_table[i].regs.eflags = 0;
    }
    process_state.next_pid = 1; // Start PIDs at 1
    process_state.current_pid = 0; // No process running initially    
    // process_state.process_table[0].pid = 1; // pid 1 means kernel
    // process_state.process_table[0].page_dir = pagedir;
    // process_state.process_table[0].state = 1;
    // process_state.process_table[0].priority = 1;
    // Kernel process doesn't need full register state here
    return 0;
}

int execute(const char *absPath, ...) {
    va_list args;
    va_start(args, absPath);
    binary_info_t bin_info = {0};
    
    // Load ELF file
    int ret = load_elf(absPath, &bin_info);
    if (ret != 0 || !fileLoaded(&bin_info)) {
        va_end(args);
        serial_printf("error loading file: -1\n");
        return -1;
    }

    // Create process
    int pid = create_process(&bin_info, args);
    if (pid < 0) {
        kfree(bin_info.segments);
        kfree(bin_info.phys_addrs);
        kfree(bin_info.sizes);
        va_end(args);
        return -2;
    }

    // Switch to process
    switch_to_process(pid);

    // Free bin_info resources
    kfree(bin_info.segments);
    kfree(bin_info.phys_addrs);
    kfree(bin_info.sizes);
    va_end(args);

    return 0; // Success
}

// int execute(const char *absPath, ...){
//     int16_t retCode=-1;
//     va_list args;
//     va_start(args, format);
//     binary_info_t bin_info;
//     loadFile(absPath, &bin_info);
//     if(!fileLoaded(&bin_info)){
//         return -1;
//     }
//     // creates process and stores it in process_table array
//     // also it should copy argv strings somewhere and then push argc and **argv 
//     // and should store _start and initial_states of the program in process so
//     // its ready to be executed
//     int pid = create_process(&bin_info, args); 
//     if(pid > 0){
//         switch_to_process(pid);
//     }
//     va_end(args);
//     return retCode;
// }

// uint32_t create_process(binary_info_t* bin) {
//     // Validate input
//     if (!bin || bin->start_addr >= bin->end_addr) return 0;

//     // Find free process slot
//     for (int i = 0; i < MAX_PROCESSES; i++) {
//         if (process_state.process_table[i].pid == 0) {
//             Process* proc = &process_state.process_table[i];
//             proc->pid = process_state.next_pid++;
//             proc->page_dir = create_process_pd();
//             proc->state = PROCESS_READY;
//             proc->priority = 1;
//             proc->time_slice = 10;

//             // Calculate size and pages for binary
//             uint32_t size = bin->end_addr - bin->start_addr;
//             uint32_t code_pages = (size + 4095) / 4096;

//             // Map code/data at 0x1000000
//             for (uint32_t j = 0; j < code_pages; j++) {
//                 assign_page_table(proc->page_dir, (void*)(0x1000000 + j * 4096),
//                                  PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
//             }

//             // Copy binary to 0x1000000
//             switch_page_directory(proc->page_dir);
//             uint32_t* user_mem = (uint32_t*)0x1000000;
//             for (uint32_t j = 0; j < size / 4; j++) {
//                 user_mem[j] = ((uint32_t*)bin->start_addr)[j];
//             }
//             // Handle remaining bytes (if size not multiple of 4)
//             for (uint32_t j = (size / 4) * 4; j < size; j++) {
//                 ((uint8_t*)0x1000000)[j] = ((uint8_t*)bin->start_addr)[j];
//             }

//             switch_page_directory(pagedir);

//             // Map 1MB stack at 0x13F0000-0x1400000
//             assign_page_table(proc->page_dir, (void*)0x13F0000,
//                              PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

//             // Kernel stack (unique per process)
//             assign_page_table(pagedir, (void*)(0xC0200000 + i * 0x4000),
//                              PAGE_PRESENT | PAGE_WRITE);
//             proc->kernel_esp = 0xC0200000 + i * 0x4000 + 0x4000;

//             // Initialize registers
//             proc->regs.eip = 0x1000000;
//             proc->regs.esp = 0x1400000;
//             proc->regs.cs = 0x1B;
//             proc->regs.ds = 0x23;
//             proc->regs.es = 0x23;
//             proc->regs.fs = 0x23;
//             proc->regs.gs = 0x23;
//             proc->regs.ss = 0x23;
//             proc->regs.eflags = 0x202;
//             proc->regs.eax = 0;
//             proc->regs.ebx = 0;
//             proc->regs.ecx = 0;
//             proc->regs.edx = 0;
//             proc->regs.esi = 0;
//             proc->regs.edi = 0;
//             proc->regs.ebp = 0;
            
//             return proc->pid;
//         }
//     }
//     return 0;
// }

static void print_module_info(multiboot_module_t *mod){
    serial_printf("\n mod_start: %08x | mod_end: %08x" , mod->mod_start, mod->mod_end);
    serial_printf(" | cmdline: %s ", mod->cmdline);
}

// static void print_module_header(ModuleHeader_t header){
//     serial_printf("\n modheader text_start: %08X | text_end: %08X | data_start: %08X | data_end: %08X", header.text_start, header.text_end, header.data_start, header.data_end );
// }


void load_multiboot_mod(multiboot_module_t* mod) {
    if (!mod || mod->mod_start >= mod->mod_end) return;
    binary_info_t bin = {
        .start_addr = mod->mod_start,
        .end_addr = mod->mod_end
    };
    // uint32_t pid = create_process(&bin);
    // if (pid) {
    //     serial_printf("Process %d created for module\n", pid);
    // }
}

multiboot_module_t* get_multiboot_mod_by_name(multiboot_info_t* mbi, const char* name) {
    if (!(mbi->flags & MULTIBOOT_INFO_MODS) || mbi->mods_count == 0 || !name) {
        serial_printf("No modules or invalid name\n");
        return;
    }
    multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        if (mods[i].cmdline && strcmp((const char*)mods[i].cmdline, name) == 0) {
            serial_printf("Found module: %s at 0x%08x\n", name, mods[i].mod_start);
            load_multiboot_mod(&mods[i]);

            return &mods[i];
        }
    }
    return;
    serial_printf("Module %s not found\n", name);
}

void load_16bit_executer(multiboot_info_t* mbi){
    multiboot_module_t *mod = 0;
    mod = get_multiboot_mod_by_name(mbi, "16bit-executer.mod");
    // print_module_info(mod);
    if (mod->mod_start >= mod->mod_end) {
        serial_printf("Invalid module\n");
        return;
    }

    uint32_t j = 0;
    uint32_t i = 0;
    /* Copy .header to 0x1000 */
    uint8_t* src = (uint8_t*)mod->mod_start;
    uint8_t* dst = (uint8_t*)0x1000;
    for(i = 0; i < mod->mod_end - mod->mod_start && i<MAX_MOD_SIZE; i++){
        dst[i] = src[i];
    }

    return;
}

static void load_fonts(multiboot_info_t* mbi){
    multiboot_module_t *mod = 0;
    mod = get_multiboot_mod_by_name(mbi, "Inconsolata-16r.psf");
    serial_printf("\n");
    print_module_info(mod);
    serial_printf("\n");
    if (mod->mod_start >= mod->mod_end) {
        serial_printf("Invalid module\n");
        return;
    }

    memset(mode_info,0,0x200);
    memset(vbe_info,0,0x200);
    memcpy(mode_info, (void *)_mbi->vbe_mode_info, sizeof(vbe_mode_info_t));
    memcpy(vbe_info, (void *)_mbi->vbe_control_info, sizeof(vbe_info_block_t));

    vbe_load_font(mod->mod_start);
}

int load_multiboot_mods(multiboot_info_t* mbi) {
    // get 16bit-executer.mod info
    load_16bit_executer(mbi);
    load_fonts(mbi);

    // }else{
    //     serial_printf("Module %s not found2\n", name);
    // }

    return 0;
}

// void load_multiboot_mods(multiboot_info_t* mbi) {
//     if (!(mbi->flags & MULTIBOOT_INFO_MODS) || mbi->mods_count == 0) return;
//     multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
//     for (uint32_t i = 0; i < mbi->mods_count; i++) {
//         load_multiboot_mod(&mods[i]);
//     }
// }

void switch_to_process(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_state.process_table[i].pid == pid && process_state.process_table[i].state == PROCESS_READY) {
            Process* proc = &process_state.process_table[i];
            process_state.current_pid = i;
            proc->state = PROCESS_RUNNING;
            switch_page_directory(proc->page_dir);

            switch_to_process_asm(&proc->regs);
            
        }
    }
}

void schedule() {
    Process* current = &process_state.process_table[process_state.current_pid];
    if (current->state == PROCESS_RUNNING) {
        current->state = PROCESS_READY;
    }

    uint32_t next_pid = (process_state.current_pid + 1) % MAX_PROCESSES;
    while (process_state.process_table[next_pid].state != PROCESS_READY && 
           next_pid != process_state.current_pid) {
        next_pid = (next_pid + 1) % MAX_PROCESSES;
    }
    if (next_pid == process_state.current_pid) return;

    Process* next = &process_state.process_table[next_pid];
    process_state.current_pid = next_pid;
    next->state = PROCESS_RUNNING;
    switch_page_directory(next->page_dir);
    schedule_asm(&next->regs);
}