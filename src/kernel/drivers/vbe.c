#include <kernel/drivers/vbe.h>
#include <kernel/multiboot.h>
#include <kernel/arch/i386/16bit-executer_wrapper.h>
#include <stdio.h>
#include <string.h>


// #ifndef FRAMEBUFFER_VADDR
#define FRAMEBUFFER_VADDR 0xD0000000
// #endif
// #ifndef VBE_INFO
#define VBE_INFO 0x9000
// #endif
// #ifndef VBE_MINFO
#define VBE_MINFO 0xA000
#define VBE_MODE_LIST 0x9400
// #endif

#define VBE_USE_BGR 1

#define _AX 0x6000
#define _BX 0x6002
#define _CX 0x6004
#define _DX 0x6006
#define _DI 0x6008
#define _ES 0x600A

uint16_t *vbe_bc_ax=(uint16_t *) _AX;
uint16_t *vbe_bc_bx=(uint16_t *) _BX;
uint16_t *vbe_bc_cx=(uint16_t *) _CX;
uint16_t *vbe_bc_dx=(uint16_t *) _DX;
uint16_t *vbe_bc_di=(uint16_t *) _DI;
uint16_t *vbe_bc_es=(uint16_t *) _ES;

extern uint32_t *_vbe_bc_text_start;
extern uint32_t *_vbe_bc_text_end;

extern uint32_t *_vbe_bc_data_start;
extern uint32_t *_vbe_bc_data_end;


// Global pointers
vbe_info_block_t* vbe_info = (vbe_info_block_t*) VBE_INFO;
vbe_mode_info_t* mode_info = (vbe_mode_info_t*) VBE_MINFO;
// uint32_t* framebuffer = (uint32_t*) FRAMEBUFFER_VADDR;
uint64_t framebuffer_addr; // framebuffer physical address or dynamic vaddr (we should map this to a fixed address later)


// External page table mapping function (you must implement)
// extern void map_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

#ifndef VBE_RUN_SIZE 
#define VBE_RUN_SIZE 0x100
#endif

uint8_t vbe_run_bin[VBE_RUN_SIZE] = {

	0xa1, 0x0a, 0x60, 0x8e, 0xc0, 0xa1, 0x00, 0x60, 
	0x8b, 0x1e, 0x02, 0x60, 0x8b, 0x0e, 0x04, 0x60, 
	0x8b, 0x16, 0x06, 0x60, 0x8b, 0x3e, 0x08, 0x60, 
	0xcd, 0x10, 0xa3, 0x00, 0x60, 0x89, 0x1e, 0x02, 
	0x60, 0x89, 0x0e, 0x04, 0x60, 0x89, 0x16, 0x06, 
	0x60, 0x89, 0x3e, 0x08, 0x60, 0x8c, 0xc0, 0xa3, 
	0x0a, 0x60, 0xa1, 0x00, 0x60, 0xc3, 

};

void print_vbe_info_block(vbe_info_block_t *info) {
    printf("VBE Info Block:\n");
    printf("Signature: %.4s\n", info->signature);
    printf("Version: %u.%u\n", info->version >> 8, info->version & 0xFF);
    printf("Total Memory: %u MB\n", info->total_memory * 64 / 1024);
    printf("Capabilities: 0x%08X\n", info->capabilities);
    printf("Video Modes Ptr: 0x%08X\n", info->video_modes);
    printf("OEM String Ptr: 0x%08X\n", info->oem_string);
    printf("Vendor Ptr: 0x%08X\n", info->vendor);
    printf("Product Name Ptr: 0x%08X\n", info->product_name);
}

void print_vbe_mode_info(vbe_mode_info_t *mode) {
    printf("VBE Mode Info:\n");
    printf("Resolution: %ux%u\n", mode->width, mode->height);
    printf("BPP: %d\n", mode->bpp);
    printf("Pitch: %u\n", mode->pitch);
    printf("Framebuffer: 0x%08X\n", mode->framebuffer);
    printf("Memory Model: 0x%02X\n", mode->memory_model);
    printf("Attributes: 0x%04X\n", mode->attributes);
    printf("Red Mask: %u (Pos: %u)\n", mode->red_mask, mode->red_position);
    printf("Green Mask: %u (Pos: %u)\n", mode->green_mask, mode->green_position);
    printf("Blue Mask: %u (Pos: %u)\n", mode->blue_mask, mode->blue_position);
}

static uint32_t vbe_bios_call(vbe_bios_call_t *bc){
    *vbe_bc_ax=bc->ax;
    *vbe_bc_bx=bc->bx;
    *vbe_bc_cx=bc->cx;
    *vbe_bc_dx=bc->dx;
    *vbe_bc_es=bc->es;
    *vbe_bc_di=bc->di;
    // *vbe_bc_di=es_di & 0xFFFF;
    // *vbe_bc_es=(es_di - *vbe_bc_di) >> 4;
    // *vbe_bc_es=es_di >> 4;
    // *vbe_bc_di= es_di - (*vbe_bc_es << 4);

    rm_binary_info_t bin_info = {
        .entry_addr = vbe_run_bin,
        .text_start_addr = vbe_run_bin,
        .text_size = sizeof(vbe_run_bin), //((uint32_t) &_vbe_bc_text_end) - ((uint32_t) &vbe_run_bin),
        .data_start_addr = 0, //&_vbe_bc_data_start,
        .data_size = 0 //set to 0 to disable memcopy //((uint32_t) &_vbe_bc_data_end) - ((uint32_t) &_vbe_bc_data_start)
    };

    uint32_t ret = call_rm_program(bin_info);
    bc->ax= *vbe_bc_ax;
    bc->bx= *vbe_bc_bx;
    bc->cx= *vbe_bc_cx;
    bc->dx= *vbe_bc_dx;
    bc->es= *vbe_bc_es;
    bc->di= *vbe_bc_di;
    return ret;    
}


// Initialize VBE and check support
int32_t vbe_init(void) {
    // Clear vbe_info and set signature
    for (int i = 0; i < sizeof(vbe_info_block_t); i++) {
        ((uint8_t*)vbe_info)[i] = 0;
    }

    vbe_bios_call_t bc = {
        .ax = 0x4F00,
        .es = 0x0900,
        .di = 0x0000
        // .ax = 0x4F00,
        // .es = 0,//(VBE_INFO >> 16) << 12 ,
        // .di = 0x9000 //VBE_INFO & 0xFFFF
    };

    // Call INT 0x10, AX=0x4F00 to get VBE info
    uint32_t res = vbe_bios_call(&bc);
    // uint32_t *res2 = 0x6000;
    // uint32_t res3 = *res2;
    if (res != 0x004F) return -1;

    // vbe_set_text_mode();
    // printf("abc res: %d %08x res2: %d %08x res2: %d %08x\n", res, res, *res2, *res2, res3, res3);

    if (res != 0x004F || vbe_info->version < 0x0200) {
        return -2; // VBE not supported or version < 2.0
    }

    if (strncmp(vbe_info->signature, "VESA", 4) != 0) return -3;

    if (!(_mbi->flags & (1 << 12))) return -5;
    framebuffer_addr = _mbi->framebuffer_addr; // Physical address
    // framebuffer = (uint32_t *) framebuffer_addr;
    // return res;
    return 0;
}


// Set a video mode
uint32_t vbe_set_mode(uint16_t width, uint16_t height, uint8_t bpp) {
    // Get mode list from vbe_info
    uint16_t* mode_list = (uint16_t*)(vbe_info->video_modes & 0xFFFF);
    uint16_t target_mode = 0x4118;

    for(int i=0; i<1; i++){
    // for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        vbe_bios_call_t bc = {
            .ax = 0x4F01,
            .cx = 0x118,
            // .cx = mode_list[i],
            .es = 0x0A00,
            .di = 0x0000
                // .es = (VBE_MINFO >> 16) << 12 ,
            // .di = VBE_MINFO & 0xFFFF
        };  
        uint32_t res;
        res = vbe_bios_call(&bc);    
        // if (res != 0x004F) continue;

        // if (mode_info->width == width && mode_info->height == height && mode_info->bpp == bpp &&
        //     (mode_info->attributes & 0x80)) { // Linear framebuffer supported
        //     target_mode = mode_list[i] | 0x4000; // Enable LFB
        //     break;
        // }
    // }
    }

    if (target_mode == 0) return -1;

    vbe_bios_call_t bc = {
        .ax = 0x4F02,
        .bx = target_mode
    };
    uint32_t res = vbe_bios_call(&bc);
    if (res != 0x004F) return -1;

    // Get mode info to verify framebuffer
    bc.ax = 0x4F01;
    bc.cx = target_mode & 0x1FFF; // Clear LFB bit
    bc.es = 0x0A00;
    bc.di = 0x0000;
    // bc.es = (VBE_MINFO >> 16) << 12 ;
    // bc.di = VBE_MINFO & 0xFFFF;
    if (vbe_bios_call(&bc) != 0x004F) return -1;

    if (!(_mbi->flags & (1 << 12))) return -1;
    framebuffer_addr = _mbi->framebuffer_addr; // Physical address
    // framebuffer = &framebuffer_addr;

    // TODO: Map to 0xD0000000 in page tables
    // map_page(0xD0000000, info.framebuffer, 0x3);
    // TODO: Map to 0xD0000000 in page tables
    // map_page(0xD0000000, info.framebuffer, 0x3);
    // return res;
    return 0;
}

// Get mode information
uint32_t vbe_get_mode_info(uint16_t mode, vbe_mode_info_t* info) {
    vbe_bios_call_t bc = {
        .ax = 0x4F01,
        .cx = mode & 0x1FFF, // Clear LFB bit
        .es = 0x0A00 ,
        .di = 0x0
        // .es = (VBE_MINFO >> 16) << 12 ,
        // .di = VBE_MINFO & 0xFFFF
    };
    int32_t res = vbe_bios_call(&bc);
    if (res != 0x004F) return -1;
    memcpy(info, mode_info, sizeof(vbe_mode_info_t));
    // return res;
    return 0;
}

// Get the current VBE mode
int vbe_get_current_mode(uint16_t *mode) {
    vbe_bios_call_t bc = {
        .ax = 0x4F03,
        // .es = (VBE_MINFO >> 16) << 12 ,
        // .di = VBE_MINFO & 0xFFFF
    };
    int32_t res = vbe_bios_call(&bc);

    if (res != 0x004F) {
        vbe_set_text_mode();
        printf("Failed to get VBE mode: AX=0x%04X\n", res);
        return -1;
    }

    *mode = *vbe_bc_bx; // BX contains the current mode
    uint16_t axx, bxx, cxx;
    axx = *vbe_bc_ax;
    bxx = *vbe_bc_bx;
    cxx = *vbe_bc_cx;
    vbe_set_text_mode();
    printf("Current VBE Mode: 0x%04X 0x%04X 0x%04X\n", axx, bxx, cxx);
    while(1);
    return 0;
}

// Map the framebuffer to virtual FRAMEBUFFER_VADDR
// commented out because in my paging setup we dont need mapping like this
// we assign premapped pagedirs to processes
// void vbe_map_framebuffer(void) {
//     uint32_t fb_addr = mode_info->framebuffer;
//     uint32_t fb_size = mode_info->height * mode_info->pitch;
//     uint32_t virt_addr = FRAMEBUFFER_VADDR;

//     // Map pages (PAGE_PRESENT | PAGE_WRITE)
//     for (uint32_t offset = 0; offset < fb_size; offset += 0x1000) {
//         map_page(fb_addr + offset, virt_addr + offset, 0x3);
//     }
// }

// Clear the screen
// void vbe_clear_screen(uint32_t color) {
//     if (!(_mbi->flags & (1 << 12))) return;
//     for (uint32_t i = 0; i < mode_info->width * mode_info->height; i++) {
//         framebuffer[i] = color;
//     }
// }
// Clear the VBE screen to a specified color
// color: 32-bit ARGB for 32-bit mode, 16-bit RGB for 16-bit mode
// mode_info: Optional, used if _mbi->flags bit 12 is unset
// Print all Multiboot1 info fields
void print_multiboot_info(multiboot_info_t *mbi) {
    printf("Multiboot1 Info Structure:\n");
    printf("  Flags: 0x%08X\n", mbi->flags);

    // Memory info (flags bit 0)
    if (mbi->flags & (1 << 0)) {
        printf("  Memory Lower: %u KB\n", mbi->mem_lower);
        printf("  Memory Upper: %u KB\n", mbi->mem_upper);
    } else {
        printf("  Memory Info: Not available (flags bit 0 unset)\n");
    }

    // Boot device (flags bit 1)
    if (mbi->flags & (1 << 1)) {
        printf("  Boot Device: 0x%08X\n", mbi->boot_device);
    } else {
        printf("  Boot Device: Not available (flags bit 1 unset)\n");
    }

    // Command line (flags bit 2)
    if (mbi->flags & (1 << 2)) {
        printf("  Command Line: 0x%08X (%s)\n", mbi->cmdline,
               mbi->cmdline ? (char *)(uintptr_t)mbi->cmdline : "Empty");
    } else {
        printf("  Command Line: Not available (flags bit 2 unset)\n");
    }

    // Modules (flags bit 3)
    if (mbi->flags & (1 << 3)) {
        printf("  Modules Count: %u\n", mbi->mods_count);
        printf("  Modules Address: 0x%08X\n", mbi->mods_addr);
    } else {
        printf("  Modules: Not available (flags bit 3 unset)\n");
    }

    // Symbol table (flags bit 4 or 5)
    if (mbi->flags & (1 << 4)) {
        printf("  Symbol Table (a.out): 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
               mbi->syms[0], mbi->syms[1], mbi->syms[2], mbi->syms[3]);
    } else if (mbi->flags & (1 << 5)) {
        printf("  Symbol Table (ELF): 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
               mbi->syms[0], mbi->syms[1], mbi->syms[2], mbi->syms[3]);
    } else {
        printf("  Symbol Table: Not available (flags bits 4 and 5 unset)\n");
    }

    // Memory map (flags bit 6)
    if (mbi->flags & (1 << 6)) {
        printf("  Memory Map Length: %u\n", mbi->mmap_length);
        printf("  Memory Map Address: 0x%08X\n", mbi->mmap_addr);
    } else {
        printf("  Memory Map: Not available (flags bit 6 unset)\n");
    }

    // Drives (flags bit 7)
    if (mbi->flags & (1 << 7)) {
        printf("  Drives Length: %u\n", mbi->drives_length);
        printf("  Drives Address: 0x%08X\n", mbi->drives_addr);
    } else {
        printf("  Drives: Not available (flags bit 7 unset)\n");
    }

    // Config table (flags bit 8)
    if (mbi->flags & (1 << 8)) {
        printf("  Config Table: 0x%08X\n", mbi->config_table);
    } else {
        printf("  Config Table: Not available (flags bit 8 unset)\n");
    }

    // Boot loader name (flags bit 9)
    if (mbi->flags & (1 << 9)) {
        printf("  Boot Loader Name: 0x%08X (%s)\n", mbi->boot_loader_name,
               mbi->boot_loader_name ? (char *)(uintptr_t)mbi->boot_loader_name : "Empty");
    } else {
        printf("  Boot Loader Name: Not available (flags bit 9 unset)\n");
    }

    // APM table (flags bit 10)
    if (mbi->flags & (1 << 10)) {
        printf("  APM Table: 0x%08X\n", mbi->apm_table);
    } else {
        printf("  APM Table: Not available (flags bit 10 unset)\n");
    }

    // VBE info (flags bit 11)
    if (mbi->flags & (1 << 11)) {
        printf("  VBE Control Info: 0x%08X\n", mbi->vbe_control_info);
        printf("  VBE Mode Info: 0x%08X\n", mbi->vbe_mode_info);
        printf("  VBE Mode: 0x%04X\n", mbi->vbe_mode);
        printf("  VBE Interface Segment: 0x%04X\n", mbi->vbe_interface_seg);
        printf("  VBE Interface Offset: 0x%04X\n", mbi->vbe_interface_off);
        printf("  VBE Interface Length: %u\n", mbi->vbe_interface_len);
    } else {
        printf("  VBE Info: Not available (flags bit 11 unset)\n");
    }

    // Framebuffer info (flags bit 12)
    if (mbi->flags & (1 << 12)) {
        framebuffer_addr = (uint32_t) mbi->framebuffer_addr;
        printf("  Framebuffer Address: 0x%08x\n", framebuffer_addr);
        printf("  Framebuffer Pitch: %u bytes\n", mbi->framebuffer_pitch);
        printf("  Framebuffer Width: %u pixels\n", mbi->framebuffer_width);
        printf("  Framebuffer Height: %u pixels\n", mbi->framebuffer_height);
        printf("  Framebuffer BPP: %u bits\n", mbi->framebuffer_bpp);
        printf("  Framebuffer Type: %u (%s)\n", mbi->framebuffer_type,
               mbi->framebuffer_type == 0 ? "Indexed" :
               mbi->framebuffer_type == 1 ? "RGB" :
               mbi->framebuffer_type == 2 ? "EGA Text" : "Unknown");
        printf("  Color Info: [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]\n",
               mbi->color_info[0], mbi->color_info[1], mbi->color_info[2],
               mbi->color_info[3], mbi->color_info[4], mbi->color_info[5]);
    } else {
        printf("  Framebuffer Info: Not available (flags bit 12 unset)\n");
    }
}

// Function to list supported VBE modes
int vbe_list_supported_modes(void) {
    // vbe_info_block_t *vbe_info = (vbe_info_block_t *)VBE_INFO;
    // vbe_mode_info_t *mode_info = (vbe_mode_info_t *)VBE_MINFO;
    uint16_t *mode_list;
    uint16_t mode;
    int i;
    int32_t res;
    vbe_set_text_mode();

    // Step 1: Initialize VBE info buffer
    vbe_info->signature[0] = 'V';
    vbe_info->signature[1] = 'E';
    vbe_info->signature[2] = 'S';
    vbe_info->signature[3] = 'A';

    // VBE Info Call (0x4F00)
    vbe_bios_call_t bc = {
        .ax = 0x4F00,
        .bx = 0,
        .cx = 0,
        .dx = 0,
        .es = VBE_INFO >> 4, // Segment
        .di = VBE_INFO & 0xF // Offset
    };
    res = vbe_bios_call(&bc);
    if (res != 0x004F || vbe_info->signature[0] != 'V') {
        // vbe_set_text_mode();
        printf("1Failed to get VBE info (AX=0x%04X) sig:%c \n", *vbe_bc_ax, vbe_info->signature[0]);
        return -1;
    }

    // Print VBE version and memory
    // vbe_set_text_mode();
    printf("VBE Version: %u.%u\n", vbe_info->version >> 8, vbe_info->version & 0xFF);
    printf("Total Video Memory: %u KB\n", vbe_info->total_memory * 64);

    // Step 2: Get mode list (segment:offset to linear)
    uint32_t mode_ptr = vbe_info->video_modes;
    uint16_t mode_seg = mode_ptr >> 16;
    uint16_t mode_off = mode_ptr & 0xFFFF;
    uint32_t linear_mode_ptr = (mode_seg << 4) + mode_off;
    mode_list = (uint16_t *)VBE_MODE_LIST;

    // Copy mode list to low memory (assume <1KB)
    // In practice, use BIOS or memory access to read mode_ptr
    // Here, simulate by assuming mode_ptr is accessible
    printf("2video_modes: %08x %08x ",vbe_info->video_modes, linear_mode_ptr );
    uint16_t tmpmode;
    int j=0;
    for(i=0; (tmpmode = ((uint16_t *)linear_mode_ptr)[i]) != 0xFFFF; i++){
        mode_list[i]= tmpmode;
    }
    mode_list[i]=0xFFFF;
    // for (i = 0; ; i++) {
    //     // Placeholder: Read mode from mode_ptr + i*2
    //     // In real code, use BIOS or copy from mode_ptr
    //     mode_list[i] = 0xFFFF; // Simulate end of list
    //     break;
    // }

    // for (i=0;mode_list[i]!=0xFFFF ;i++){
    //     printf("|mode:%04x ", mode_list[i]);
    // }
    // while(1);

    // Step 3: Iterate through modes
    // vbe_set_text_mode();
    printf("Supported VBE Modes:\n");
    printf("Mode|WidthxHeight|BPP|Memory Model|Color Positions(R,G,B)|Framebuffer|LFB\n");
    printf("----|-------------|----|------------|--------------------|------------|----\n");

    for (i = 0; mode_list[i] != 0xFFFF; i++) {
        mode = mode_list[i];
        // if(i>40)
        //     while(1);

        // VBE Mode Info Call (0x4F01)
        bc.ax = 0x4F01;
        bc.cx = mode;
        bc.es = VBE_MINFO >> 4;
        bc.di = VBE_MINFO & 0xF;
        bc.bx = 0;
        bc.dx = 0;

        res = vbe_bios_call(&bc);
        if (res != 0x004F) {
            // vbe_set_text_mode();
            printf("Failed to get info for mode 0x%04X (AX=0x%04X)\n", mode, res);
            continue;
        }

        // Filter for graphical modes (memory model 4=packed pixel, 6=direct color)
        if (mode_info->memory_model != 4 && mode_info->memory_model != 6) {
            continue;
        }

        // Filter for supported bpp (16, 24, 32)
        if (mode_info->bpp != 16 && mode_info->bpp != 24 && mode_info->bpp != 32) {
            continue;
        }

        // Check LFB support
        int lfb_supported = (mode_info->attributes & 0x80) ? 1 : 0;

        if(
            mode_info->bpp<24 ||
            mode_info->width<800 ||
            mode_info->height<600 ||
            mode_info->width>1280 ||
            mode_info->height >1080
        ){
            continue;
        }


        // Print mode details
        // vbe_set_text_mode();
        printf("%04X|%dx",
            mode,
            mode_info->width,
            mode_info->width
        );
        printf("%d|%d|",
            mode_info->height,
            mode_info->bpp
        );
        printf("%s|%d,",
            mode_info->memory_model == 4 ? "PP" : mode_info->memory_model == 6 ? "DC" : "Unknown",// packed pixle / direct color
            mode_info->red_position
        );
        printf("%d,%d|",
            mode_info->green_position,
            mode_info->blue_position
        );
        printf("%04X|%s>",
            mode_info->framebuffer>>16,
            lfb_supported ? "Yes" : "No"
        );
        if(j++%2)
            printf("\n");

        // printf("0x%04X|%ux%u|%u|%s|%u,%u,%u|0x%08X|%s\n",
        //     mode,
        //     mode_info->width,
        //     mode_info->height,
        //     mode_info->bpp,
        //     mode_info->memory_model == 4 ? "Packed Pixel" :
        //     mode_info->memory_model == 6 ? "Direct Color" : "Unknown",
        //     mode_info->red_position,
        //     mode_info->green_position,
        //     mode_info->blue_position,
        //     mode_info->framebuffer,
        //     lfb_supported ? "Yes" : "No");
    }

    // vbe_set_text_mode();
    // printf("\nNotes:\n");
    // printf("- Use modes with LFB for vbe_clear_screen.\n");
    // printf("- BPP 24 requires VBE_USE_BGR=1 for BGR (Red=0, Green=16, Blue=8).\n");
    // printf("- QEMU (-vga std) modes: 0x115/0x415 (1024x768x24), 0x118/0x418 (1024x768x32).\n");
    return 0;
}

int vbe_clear_screen(uint32_t color/*, vbe_mode_info_t *mode_info*/) {

    // Step 1: Get framebuffer info from _mbi or mode_info
    if (_mbi->flags & (1 << 12)) {
        framebuffer.fb_addr = (uint32_t) _mbi->framebuffer_addr;
        framebuffer.width = _mbi->framebuffer_width;
        framebuffer.height = _mbi->framebuffer_height;
        framebuffer.pitch = _mbi->framebuffer_pitch;
        framebuffer.bpp = _mbi->framebuffer_bpp; 

        // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        // print_multiboot_info(_mbi);
        // printf("fb_addr: %08x width: %d height: %d pitch: %d bpp: %d ", framebuffer.fb_addr, framebuffer.width, framebuffer.height, framebuffer.pitch, framebuffer.bpp);
        // while(1);
    }/* else if (mode_info != NULL) {
        framebuffer.fb_addr = mode_info->framebuffer;
        framebuffer.width = mode_info->width;
        framebuffer.height = mode_info->height;
        framebuffer.pitch = mode_info->pitch;
        framebuffer.bpp = mode_info->bpp;
        // printf("fb_addr: %08x width: %d height: %d pitch: %d bpp: %d ", fb_addr, width, height, pitch, bpp);
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    //     printf(" 222 ");
    }*/ else {
        if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        printf("No framebuffer info available\n");
        return -1;
    }

    // Step 2: Validate bpp (16 or 32)
    // if (/*bpp != 16 && */bpp != 32) {
    //     printf("Unsupported bpp: %u\n", bpp);
    //     return -1;
    // }

    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // print_multiboot_info(_mbi);

    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // printf("fb_addr: %08x width: %d ", fb_addr, width, height, bpp);
    // printf("height: %d bpp: %d", height, bpp);
    // printf("pitch: %u", pitch);
    // while(1);
    // Step 3: Clear the screen
    uint8_t *fb = (uint8_t *)(uintptr_t)framebuffer.fb_addr;
    uint32_t row, col;
    // bpp=24;
    if (framebuffer.bpp == 32) {
        uint32_t *fb32 = (uint32_t *)fb;
        for (row = 0; row < framebuffer.height; row++) {
            for (col = 0; col < framebuffer.width; col++) {
                fb32[row * (framebuffer.pitch / 4) + col] = color;
            }
        }
    }else if (framebuffer.bpp == 24) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        for (row = 0; row < framebuffer.height; row++) {
            uint8_t *row_ptr = fb + row * framebuffer.pitch;
            for (col = 0; col < framebuffer.width; col++) {
#if VBE_USE_BGR
                row_ptr[col * 3 + 0] = b; // Blue
                row_ptr[col * 3 + 1] = g; // Green
                row_ptr[col * 3 + 2] = r; // Red
#else
                row_ptr[col * 3 + 0] = r; // Red
                row_ptr[col * 3 + 1] = g; // Green
                row_ptr[col * 3 + 2] = b; // Blue
#endif
            }
        }
    } else { // bpp == 16
        uint16_t *fb16 = (uint16_t *)fb;
        uint16_t color16 = (uint16_t)color; // Truncate to 16-bit
        for (row = 0; row < framebuffer.height; row++) {
            for (col = 0; col < framebuffer.width; col++) {
                fb16[row * (framebuffer.pitch / 2) + col] = color16;
            }
        }
    }
    // while(1);
    return 0;
}

int vbe_set_text_mode(void) {
    vbe_bios_call_t bc = {
        .ax = 0x0003
    };
    int32_t res = vbe_bios_call(&bc);
    if (vbe_is_text_mode() != 1) return -1;
    return 0;
    // return (res == 0x004F || res == 0) ? 0 : -1;
}


int vbe_is_text_mode(void) {
    vbe_bios_call_t bc = {
        .ax = 0x0F03  // Function 0x0F: Get current video mode
    };
    int32_t res = vbe_bios_call(&bc);
    uint8_t mode = res & 0xFF; // AL = video mode
    return (mode == 0x03 || mode == 0x02 || mode == 0x07) ? 1 : 0;
}

// Draw a pixel
void vbe_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if(x >= framebuffer.width || x < 0 || y>=framebuffer.height || y<0){
        while(1);
        return;
    }
    uint8_t *fb = (uint8_t *)(uintptr_t)framebuffer.fb_addr;
    if(framebuffer.bpp == 32){
        uint32_t *fb32 = (uint32_t *)fb;
        fb32[y * (framebuffer.pitch / 4) + x] = color;
        if (x < framebuffer.width && y < framebuffer.height) {
            uint32_t offset = y * (framebuffer.pitch / 4) + x;
            fb32[y * (framebuffer.pitch / 4) + x] = color;
        }
    }else if(framebuffer.bpp == 24){
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        uint8_t *row_ptr = fb + y * framebuffer.pitch;
#if VBE_USE_BGR
        row_ptr[x * 3 + 0] = b; // Blue
        row_ptr[x * 3 + 1] = g; // Green
        row_ptr[x * 3 + 2] = r; // Red
#else
        row_ptr[x * 3 + 0] = r; // Red
        row_ptr[x * 3 + 1] = g; // Green
        row_ptr[x * 3 + 2] = b; // Blue
#endif

    }
// if (x < framebuffer.width && y < framebuffer.height) {
//     uint32_t offset = y * (framebuffer.pitch / 4) + x;
//     framebuffer.fb_addr[offset] = color;
// }
}

// Fill a rectangle
void vbe_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    uint8_t *fb = (uint8_t *)(uintptr_t)framebuffer.fb_addr;

    for (uint16_t j = y; j < y + height && j < height; j++) {
        for (uint16_t i = x; i < x + width && i < width; i++) {
            uint32_t offset = j * (framebuffer.pitch / 4) + i;
            fb[offset] = color;
        }
    }
}