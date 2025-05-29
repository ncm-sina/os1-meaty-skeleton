// #include <kernel/process.h>
// #include <kernel/arch/i386/paging.h>
// #include <stdio.h>
// #include <string.h>
// #include <kernel/drivers/vbe.h>
// #include <kernel/drivers/serial.h>

#include <kernel/process.h>
#include <kernel/binfmt/elf.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
// #include <memory.h>
#include <kernel/drivers/serial.h>


#define MAX_MOD_SIZE 0xFFFFF

extern uint32_t pagedir[1024];
// extern vbe_info_block_t* vbe_info;
// extern vbe_mode_info_t* mode_info;


ProcessState process_state;
Process *current_process;

// ProcessState process_state = {
//     .process_table = {{0}},
//     .next_pid = 1,
//     .current_pid = 0
// };

// Internal helpers
static int fileLoaded(binary_info_t *bin_info) {
    return bin_info->start_addr != 0 && bin_info->end_addr > bin_info->start_addr;
}

static int elfFileLoaded(elf_info_t *elf_info) {
    return elf_info->num_segments > 0 && elf_info->entry_point != 0;
}

// static uint32_t copy_argv_to_stack(uint32_t *stack_top, va_list args, int *argc_out) {
//     char **argv = NULL;
//     int argc = 0;

//     va_list args_copy;
//     va_copy(args_copy, args);
//     while (va_arg(args_copy, char *) != NULL) {
//         argc++;
//     }
//     va_end(args_copy);

//     if (argc > 0) {
//         argv = kmalloc(argc * sizeof(char *));
//         va_list args_copy2;
//         va_copy(args_copy2, args);
//         for (int i = 0; i < argc; i++) {
//             const char *arg = va_arg(args_copy2, char *);
//             uint32_t len = strlen(arg) + 1;
//             uint32_t phys_addr = allocate_physical_pages(len);
//             map_pages(current_process->page_dir, *stack_top - len, phys_addr, len, USER_READ_WRITE);
//             memcpy((void *)phys_addr, arg, len);
//             argv[i] = (char *)(*stack_top - len);
//             *stack_top -= len;
//         }
//         va_end(args_copy2);
//     }

//     *stack_top &= ~0xF;

//     for (int i = argc - 1; i >= 0; i--) {
//         *stack_top -= sizeof(char *);
//         map_pages(current_process->page_dir, *stack_top, allocate_physical_pages(sizeof(char *)), sizeof(char *), USER_READ_WRITE);
//         *(char **)(*stack_top) = argv[i];
//     }

//     *stack_top -= sizeof(int);
//     map_pages(current_process->page_dir, *stack_top, allocate_physical_pages(sizeof(int)), sizeof(int), USER_READ_WRITE);
//     *(int *)(*stack_top) = argc;

//     *stack_top -= sizeof(uint32_t);
//     map_pages(current_process->page_dir, *stack_top, allocate_physical_pages(sizeof(uint32_t)), sizeof(uint32_t), USER_READ_WRITE);
//     *(uint32_t *)(*stack_top) = 0;

//     *argc_out = argc;
//     if (argv) kfree(argv);
//     return *stack_top;
// }

// void process_init() {
//     for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
//         process_state.process_table[i].state = PROCESS_FREE;
//         process_state.process_table[i].pid = 0;
//         process_state.process_table[i].page_dir = NULL;
//         process_state.process_table[i].priority = 0;
//         process_state.process_table[i].time_slice = 0;
//         process_state.process_table[i].kernel_esp = 0;
//         process_state.process_table[i].regs.eax = 0;
//         process_state.process_table[i].regs.ebx = 0;
//         process_state.process_table[i].regs.ecx = 0;
//         process_state.process_table[i].regs.edx = 0;
//         process_state.process_table[i].regs.esi = 0;
//         process_state.process_table[i].regs.edi = 0;
//         process_state.process_table[i].regs.ebp = 0;
//         process_state.process_table[i].regs.esp = 0;
//         process_state.process_table[i].regs.eip = 0;
//         process_state.process_table[i].regs.cs = 0;
//         process_state.process_table[i].regs.ds = 0;
//         process_state.process_table[i].regs.es = 0;
//         process_state.process_table[i].regs.fs = 0;
//         process_state.process_table[i].regs.gs = 0;
//         process_state.process_table[i].regs.ss = 0;
//         process_state.process_table[i].regs.eflags = 0;
//     }
//     process_state.next_pid = 1;
//     process_state.current_pid = 0;
// }

// static int create_flat_process(binary_info_t *bin_info, va_list args) {
//     int pid = -1;
//     for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
//         if (process_state.process_table[i].state == PROCESS_FREE) {
//             pid = process_state.next_pid++;
//             process_state.process_table[i].pid = pid;
//             break;
//         }
//     }
//     if (pid == -1) return -1;

//     Process *proc = &process_state.process_table[pid % MAX_PROCESSES];
//     current_process = proc;

//     proc->page_dir = allocate_page_directory();
//     if (!proc->page_dir) return -2;

//     map_kernel_space(proc->page_dir);

//     map_pages(proc->page_dir, USER_CODE_ADDR, bin_info->start_addr, 
//               bin_info->end_addr - bin_info->start_addr, USER_READ_WRITE | USER_EXEC);

//     proc->state = PROCESS_READY;
//     proc->priority = 1;
//     proc->time_slice = 10;

//     uint32_t kernel_stack_phys = allocate_physical_pages(KERNEL_STACK_SIZE);
//     map_pages(proc->page_dir, 0xC0000000 - KERNEL_STACK_SIZE, kernel_stack_phys, KERNEL_STACK_SIZE, KERNEL_READ_WRITE);
//     proc->kernel_esp = 0xC0000000 - sizeof(uint32_t);

//     uint32_t user_stack_phys = allocate_physical_pages(USER_STACK_SIZE);
//     uint32_t user_stack_top = USER_STACK_TOP;
//     map_pages(proc->page_dir, user_stack_top - USER_STACK_SIZE, user_stack_phys, USER_STACK_SIZE, USER_READ_WRITE);

//     int argc;
//     user_stack_top = copy_argv_to_stack(&user_stack_top, args, &argc);

//     proc->regs.eax = proc->regs.ebx = proc->regs.ecx = proc->regs.edx = 0;
//     proc->regs.esi = proc->regs.edi = proc->regs.ebp = 0;
//     proc->regs.esp = user_stack_top;
//     proc->regs.eip = USER_CODE_ADDR; // Assume flat binary entry point
//     proc->regs.cs = USER_CODE_SELECTOR | 3;
//     proc->regs.ds = proc->regs.es = proc->regs.fs = proc->regs.gs = USER_DATA_SELECTOR | 3;
//     proc->regs.ss = USER_DATA_SELECTOR | 3;
//     proc->regs.eflags = 0x202;

//     return pid;
// }

// static int create_elf_process(elf_info_t *elf_info, va_list args) {
//     int pid = -1;
//     for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
//         if (process_state.process_table[i].state == PROCESS_FREE) {
//             pid = process_state.next_pid++;
//             process_state.process_table[i].pid = pid;
//             break;
//         }
//     }
//     if (pid == -1) return -1;

//     Process *proc = &process_state.process_table[pid % MAX_PROCESSES];
//     current_process = proc;

//     proc->page_dir = allocate_page_directory();
//     if (!proc->page_dir) return -2;

//     map_kernel_space(proc->page_dir);

//     proc->state = PROCESS_READY;
//     proc->priority = 1;
//     proc->time_slice = 10;

//     uint32_t kernel_stack_phys = allocate_physical_pages(KERNEL_STACK_SIZE);
//     map_pages(proc->page_dir, 0xC0000000 - KERNEL_STACK_SIZE, kernel_stack_phys, KERNEL_STACK_SIZE, KERNEL_READ_WRITE);
//     proc->kernel_esp = 0xC0000000 - sizeof(uint32_t);

//     uint32_t user_stack_phys = allocate_physical_pages(USER_STACK_SIZE);
//     uint32_t user_stack_top = USER_STACK_TOP;
//     map_pages(proc->page_dir, user_stack_top - USER_STACK_SIZE, user_stack_phys, USER_STACK_SIZE, USER_READ_WRITE);

//     int argc;
//     user_stack_top = copy_argv_to_stack(&user_stack_top, args, &argc);

//     proc->regs.eax = proc->regs.ebx = proc->regs.ecx = proc->regs.edx = 0;
//     proc->regs.esi = proc->regs.edi = proc->regs.ebp = 0;
//     proc->regs.esp = user_stack_top;
//     proc->regs.eip = elf_info->entry_point;
//     proc->regs.cs = USER_CODE_SELECTOR | 3;
//     proc->regs.ds = proc->regs.es = proc->regs.fs = proc->regs.gs = USER_DATA_SELECTOR | 3;
//     proc->regs.ss = USER_DATA_SELECTOR | 3;
//     proc->regs.eflags = 0x202;

//     return pid;
// }

// static int is_elf_file(const char *absPath) {
//     uint8_t *elf_data;
//     uint32_t elf_size;
//     if (loadFile(absPath, &elf_data, &elf_size) != 0) {
//         return 0;
//     }
//     elf_header_t *header = (elf_header_t *)elf_data;
//     int is_elf = validate_elf(header) == 0;
//     kfree(elf_data);
//     return is_elf;
// }

// int execute(const char *absPath, ...) {
//     va_list args;
//     va_start(args, absPath);

//     if (is_elf_file(absPath)) {
//         elf_info_t elf_info = {0};
//         int ret = load_elf(absPath, &elf_info);
//         if (ret != 0 || !elfFileLoaded(&elf_info)) {
//             va_end(args);
//             return -1;
//         }

//         int pid = create_elf_process(&elf_info, args);
//         if (pid < 0) {
//             kfree(elf_info.segments);
//             kfree(elf_info.phys_addrs);
//             kfree(elf_info.sizes);
//             va_end(args);
//             return -2;
//         }

//         switch_to_process(pid);

//         kfree(elf_info.segments);
//         kfree(elf_info.phys_addrs);
//         kfree(elf_info.sizes);
//         va_end(args);
//         return 0;
//     } else {
//         binary_info_t bin_info = {0};
//         if (loadFile(absPath, (void*)&bin_info, NULL) != 0 || !fileLoaded(&bin_info)) {
//             va_end(args);
//             return -1;
//         }

//         int pid = create_flat_process(&bin_info, args);
//         if (pid < 0) {
//             va_end(args);
//             return -2;
//         }

//         switch_to_process(pid);
//         va_end(args);
//         return 0;
//     }
// }

// void switch_to_process(uint32_t pid) {
//     Process *proc = &process_state.process_table[pid % MAX_PROCESSES];
//     if (proc->state != PROCESS_READY && proc->state != PROCESS_RUNNING) {
//         return;
//     }

//     if (process_state.current_pid != 0) {
//         Process *current = &process_state.process_table[process_state.current_pid % MAX_PROCESSES];
//         current->state = PROCESS_READY;
//     }

//     process_state.current_pid = pid;
//     proc->state = PROCESS_RUNNING;

//     asm volatile ("mov %0, %%cr3" : : "r"(proc->page_dir));
//     asm volatile ("mov %0, %%esp" : : "r"(proc->kernel_esp));

//     asm volatile (
//         "mov %0, %%eax\n"
//         "mov %%eax, %%ds\n"
//         "mov %%eax, %%es\n"
//         "mov %%eax, %%fs\n"
//         "mov %%eax, %%gs\n"
//         "pushl %0\n"
//         "pushl %1\n"
//         "pushf\n"
//         "popl %%eax\n"
//         "orl $0x200, %%eax\n"
//         "pushl %%eax\n"
//         "pushl %2\n"
//         "pushl %3\n"
//         "iret"
//         :
//         : "r"(USER_DATA_SELECTOR | 3), "r"(proc->regs.esp), "r"(USER_CODE_SELECTOR | 3), "r"(proc->regs.eip)
//         : "eax"
//     );
// }

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

// static void load_fonts(multiboot_info_t* mbi){
//     multiboot_module_t *mod = 0;
//     mod = get_multiboot_mod_by_name(mbi, "Inconsolata-16r.psf");
//     serial_printf("\n");
//     print_module_info(mod);
//     serial_printf("\n");
//     if (mod->mod_start >= mod->mod_end) {
//         serial_printf("Invalid module\n");
//         return;
//     }

//     memset(mode_info,0,0x200);
//     memset(vbe_info,0,0x200);
//     memcpy(mode_info, (void *)_mbi->vbe_mode_info, sizeof(vbe_mode_info_t));
//     memcpy(vbe_info, (void *)_mbi->vbe_control_info, sizeof(vbe_info_block_t));

//     vbe_load_font(mod->mod_start);
// }

int load_multiboot_mods(multiboot_info_t* mbi) {
    // get 16bit-executer.mod info
    load_16bit_executer(mbi);
    // load_fonts(mbi);

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

// void switch_to_process(uint32_t pid) {
//     for (int i = 0; i < MAX_PROCESSES; i++) {
//         if (process_state.process_table[i].pid == pid && process_state.process_table[i].state == PROCESS_READY) {
//             Process* proc = &process_state.process_table[i];
//             process_state.current_pid = i;
//             proc->state = PROCESS_RUNNING;
//             switch_page_directory(proc->page_dir);

//             switch_to_process_asm(&proc->regs);
            
//         }
//     }
// }

// void schedule() {
//     Process* current = &process_state.process_table[process_state.current_pid];
//     if (current->state == PROCESS_RUNNING) {
//         current->state = PROCESS_READY;
//     }

//     uint32_t next_pid = (process_state.current_pid + 1) % MAX_PROCESSES;
//     while (process_state.process_table[next_pid].state != PROCESS_READY && 
//            next_pid != process_state.current_pid) {
//         next_pid = (next_pid + 1) % MAX_PROCESSES;
//     }
//     if (next_pid == process_state.current_pid) return;

//     Process* next = &process_state.process_table[next_pid];
//     process_state.current_pid = next_pid;
//     next->state = PROCESS_RUNNING;
//     switch_page_directory(next->page_dir);
//     schedule_asm(&next->regs);
// }