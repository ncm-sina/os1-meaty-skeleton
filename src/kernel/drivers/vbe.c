#include <kernel/drivers/vbe.h>
#include <kernel/multiboot.h>
#include <kernel/arch/i386/16bit-executer_wrapper.h>
#include <stdio.h>
#include <string.h>


#define FRAMEBUFFER_VADDR 0xD0000000

#define VBE_INFO 0x9000

#define VBE_MINFO 0x9200
#define VBE_MODE_LIST 0x9500

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



typedef struct {
    uint16_t magic;
    uint8_t mode;
    uint8_t charsize;
} psf1_header_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
} psf2_header_t;

typedef struct {
    uint8_t data[16][24]; // 8x16 characters, 3 bytes per pixel
} prerendered_char_t;

// Global pointers
vbe_info_block_t* vbe_info = (vbe_info_block_t*) VBE_INFO;
vbe_mode_info_t* mode_info = (vbe_mode_info_t*) VBE_MINFO;

// uint32_t* framebuffer = (uint32_t*) FRAMEBUFFER_VADDR;
uint64_t framebuffer_addr; // framebuffer physical address or dynamic vaddr (we should map this to a fixed address later)


uint8_t *font_bitmap = (uint8_t *)0xD1000000;
// uint8_t *font_char_bitmap = (uint8_t *)0xD110000;
int num_chars;
int char_height;
int char_width;
int bytes_per_glyph;
uint8_t unicode_to_glyph[65536];
prerendered_char_t prerendered_chars[512];
color_t fg_color = {255, 255, 255};
color_t bg_color = {0, 0, 0};
int cursor_col = 0;
int cursor_row = 0;

int max_col;
int max_row;

// extern uint16_t screen_width;
// extern uint16_t screen_height;
// extern uint8_t *frame_buffer;
// extern uint16_t pitch;


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
    serial_printf("VBE Info Block:\n");
    serial_printf("Signature: %.4s\n", info->signature);
    serial_printf("Version: %u.%u\n", info->version >> 8, info->version & 0xFF);
    serial_printf("Total Memory: %u MB\n", info->total_memory * 64 / 1024);
    serial_printf("Capabilities: 0x%08x\n", info->capabilities);
    serial_printf("Video Modes Ptr: 0x%08x\n", info->video_modes);
    serial_printf("OEM String Ptr: 0x%08x\n", info->oem_string);
    serial_printf("Vendor Ptr: 0x%08x\n", info->vendor);
    serial_printf("Product Name Ptr: 0x%08x\n", info->product_name);
}

void print_vbe_mode_info(vbe_mode_info_t *mode) {
    serial_printf("VBE Mode Info:\n");
    serial_printf("Resolution: %ux%u\n", mode->width, mode->height);
    serial_printf("BPP: %d\n", mode->bpp);
    serial_printf("Pitch: %u\n", mode->pitch);
    serial_printf("Framebuffer: 0x%08x\n", mode->framebuffer);
    serial_printf("Memory Model: 0x%02x\n", mode->memory_model);
    serial_printf("Attributes: 0x%04x\n", mode->attributes);
    serial_printf("Red Mask: %u (Pos: %u)\n", mode->red_mask, mode->red_position);
    serial_printf("Green Mask: %u (Pos: %u)\n", mode->green_mask, mode->green_position);
    serial_printf("Blue Mask: %u (Pos: %u)\n", mode->blue_mask, mode->blue_position);
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
    serial_printf("framebuffer_addr1: %x", framebuffer_addr);
    // *mode_info = (vbe_mode_info_t) _mbi->vbe_mode_info;
    // *vbe_info =  (vbe_info_block_t) _mbi->vbe_control_info;
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // printf("vbe_mode_info:%08x %08x| mode_info:%08x %08x ", _mbi->vbe_mode_info, (char *)(uintptr_t)_mbi->vbe_mode_info, mode_info, *mode_info);
    // print_vbe_mode_info(mode_info);
    // while(1);
    // framebuffer = (uint32_t *) framebuffer_addr;
    // return res;
    return 0;
}

uint32_t vbe_set_mode(uint16_t mode) {
    // mode = mode | 0x4000;
    vbe_bios_call_t bc = {
        .ax = 0x4F02,
        .bx = mode
    };
    uint32_t res = vbe_bios_call(&bc);
    if (res != 0x004F) return -1;

    // Get mode info to verify framebuffer
    bc.ax = 0x4F01;
    bc.cx = mode & 0x1FFF; // Clear LFB bit
    bc.es = 0x0920;
    bc.di = 0x0000;
    // bc.es = (VBE_MINFO >> 16) << 12 ;
    // bc.di = VBE_MINFO & 0xFFFF;
    if (vbe_bios_call(&bc) != 0x004F) return -2;

    if (!(_mbi->flags & (1 << 12))) return -3;
    // print_vbe_mode_info(mode_info);
    framebuffer_addr = mode_info->framebuffer; // Physical address
    serial_printf("framebuffer_addr2: %x", framebuffer_addr);
    // framebuffer = &framebuffer_addr;

    return 0;
}


// Set a video mode
uint32_t vbe_set_mode2(uint16_t width, uint16_t height, uint8_t bpp) {
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
//     uint32_t framebuffer = mode_info->framebuffer;
//     uint32_t fb_size = mode_info->height * mode_info->pitch;
//     uint32_t virt_addr = FRAMEBUFFER_VADDR;

//     // Map pages (PAGE_PRESENT | PAGE_WRITE)
//     for (uint32_t offset = 0; offset < fb_size; offset += 0x1000) {
//         map_page(framebuffer + offset, virt_addr + offset, 0x3);
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
    serial_printf("Supported VBE Modes:\n");
    serial_printf("Mode|WidthxHeight|BPP|Memory Model|Color Positions(R,G,B)|Framebuffer|LFB\n");
    serial_printf("----|-------------|----|------------|--------------------|------------|----\n");

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
            serial_printf("Failed to get info for mode 0x%04X (AX=0x%04X)\n", mode, res);
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
            mode_info->width>1600 ||
            mode_info->height >1080
        ){
            continue;
        }


        // Print mode details
        // vbe_set_text_mode();
        serial_printf("%04x|%dx",
            mode,
            mode_info->width,
            mode_info->width
        );
        serial_printf("%d|%d|",
            mode_info->height,
            mode_info->bpp
        );
        serial_printf("%s|%d,",
            mode_info->memory_model == 4 ? "PP" : mode_info->memory_model == 6 ? "DC" : "Unknown",// packed pixle / direct color
            mode_info->red_position
        );
        serial_printf("%d,%d|",
            mode_info->green_position,
            mode_info->blue_position
        );
        serial_printf("%04x|%s>",
            mode_info->framebuffer>>16,
            lfb_supported ? "Yes" : "No"
        );
        // if(j++%2)
            serial_printf("\n");

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
        // mode_info->framebuffer = (uint32_t) _mbi->framebuffer_addr;
        // mode_info->width = _mbi->framebuffer_width;
        // mode_info->height = _mbi->framebuffer_height;
        // mode_info->pitch = _mbi->framebuffer_pitch;
        // mode_info->bpp = _mbi->framebuffer_bpp; 

        // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        // print_multiboot_info(_mbi);
        // printf("framebuffer: %08x width: %d height: %d pitch: %d bpp: %d ", mode_info->framebuffer, mode_info->width, mode_info->height, mode_info->pitch, mode_info->bpp);
        // while(1);
    }/* else if (mode_info != NULL) {
        mode_info->framebuffer = mode_info->framebuffer;
        mode_info->width = mode_info->width;
        mode_info->height = mode_info->height;
        mode_info->pitch = mode_info->pitch;
        mode_info->bpp = mode_info->bpp;
        // printf("framebuffer: %08x width: %d height: %d pitch: %d bpp: %d ", framebuffer, width, height, pitch, bpp);
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
    // printf("framebuffer: %08x width: %d ", framebuffer, width, height, bpp);
    // printf("height: %d bpp: %d", height, bpp);
    // printf("pitch: %u", pitch);
    // while(1);
    // Step 3: Clear the screen
    uint8_t *fb = (uint8_t *)(uintptr_t)mode_info->framebuffer;
    uint32_t row, col;
    // bpp=24;
    if (mode_info->bpp == 32) {
        uint32_t *fb32 = (uint32_t *)fb;
        for (row = 0; row < mode_info->height; row++) {
            for (col = 0; col < mode_info->width; col++) {
                fb32[row * (mode_info->pitch / 4) + col] = color;
            }
        }
    }else if (mode_info->bpp == 24) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        for (row = 0; row < mode_info->height; row++) {
            uint8_t *row_ptr = fb + row * mode_info->pitch;
            for (col = 0; col < mode_info->width; col++) {
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
        for (row = 0; row < mode_info->height; row++) {
            for (col = 0; col < mode_info->width; col++) {
                fb16[row * (mode_info->pitch / 2) + col] = color16;
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
    if(x >= mode_info->width || x < 0 || y>=mode_info->height || y<0){
        if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        printf("err fb:%08x w:%08x h:%08x x:%d y:%d color:%08x", mode_info->framebuffer, mode_info->width, mode_info->height, x,y,color );
        while(1);
        return;
    }
    uint8_t *fb = (uint8_t *)(uintptr_t)mode_info->framebuffer;
    if(mode_info->bpp == 32){
        uint32_t *fb32 = (uint32_t *)fb;
        fb32[y * (mode_info->pitch / 4) + x] = color;
        if (x < mode_info->width && y < mode_info->height) {
            uint32_t offset = y * (mode_info->pitch / 4) + x;
            fb32[y * (mode_info->pitch / 4) + x] = color;
        }
    }else if(mode_info->bpp == 24){
        // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        uint8_t *row_ptr = fb + y * mode_info->pitch;
        // printf("(%08x %d %d %08x)",mode_info->pitch, x, y, color);
        // while(1);
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
// if (x < mode_info->width && y < mode_info->height) {
//     uint32_t offset = y * (mode_info->pitch / 4) + x;
//     mode_info->framebuffer[offset] = color;
// }
}

// Fill a rectangle
void vbe_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    uint8_t *fb = (uint8_t *)(uintptr_t)mode_info->framebuffer;

    for (uint16_t j = y; j < y + height && j < height; j++) {
        for (uint16_t i = x; i < x + width && i < width; i++) {
            uint32_t offset = j * (mode_info->pitch / 4) + i;
            fb[offset] = color;
        }
    }
}

static int utf8_sequence_length(uint8_t byte) {
    if (byte < 0x80) return 1;
    else if ((byte & 0xE0) == 0xC0) return 2;
    else if ((byte & 0xF0) == 0xE0) return 3;
    else if ((byte & 0xF8) == 0xF0) return 4;
    else return 1; // invalid
}

static uint32_t read_utf8(uint8_t **p) {
    uint8_t *s = *p;
    if (*s < 0x80) {
        *p += 1;
        return *s;
    } else if ((*s & 0xE0) == 0xC0) {
        uint32_t cp = ((*s & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
        return cp;
    } else if ((*s & 0xF0) == 0xE0) {
        uint32_t cp = ((*s & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *p += 3;
        return cp;
    } else if ((*s & 0xF8) == 0xF0) {
        uint32_t cp = ((*s & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *p += 4;
        return cp;
    } else {
        *p += 1;
        return *s; // invalid
    }
}

uint32_t color_to_uint32(color_t c) {
    return (c.r << 16) | (c.g << 8) | c.b;
}

// void draw_char_at(char c, int x, int y) {
//     // printf("drawcharat: %c %d %d ", c, x, y);
//     uint8_t code = (uint8_t)c;
//     uint8_t glyph = unicode_to_glyph[code];
//     if (glyph == 255) {
//         glyph = unicode_to_glyph['?'];
//         if (glyph == 255) return;
//     }
//     uint8_t *char_bitmap = font_bitmap + glyph * bytes_per_glyph;
//     for (int i = 0; i < char_height; i++) {
//         uint8_t row = char_bitmap[i];
//         for (int j = 0; j < char_width; j++) {
//             int bit = (row >> (7 - j)) & 1;
//             uint32_t color = bit ? color_to_uint32(fg_color) : color_to_uint32(bg_color);
//             // printf("(%d %d, %08x)",x + j, y + i, color );
//             // vga_put_char_at(2+j,2+i,color?'0':'.');
//             vbe_draw_pixel(x + j, y + i, color);
//         }
//     }
// }

void draw_char_at(char c, int x, int y) {
    uint8_t code = (uint8_t)c;
    uint8_t glyph = unicode_to_glyph[code];
    if (glyph == 255) {
        glyph = unicode_to_glyph['?'];
        if (glyph == 255) return;
    }
    prerendered_char_t *char_data = &prerendered_chars[glyph];
    uint8_t *dest = mode_info->framebuffer + y * mode_info->pitch + x * 3;
    for (int i = 0; i < char_height; i++) {
        memcpy(dest, char_data->data[i], char_width * 3);
        dest += mode_info->pitch;
    }
}




void scroll_up() {
    memmove(mode_info->framebuffer, mode_info->framebuffer + mode_info->pitch * char_height, mode_info->pitch * (mode_info->height - char_height));
    for (int i = mode_info->height - char_height; i < mode_info->height; i++) {
        uint8_t *row = mode_info->framebuffer + i * mode_info->pitch;
        for (int j = 0; j < mode_info->width; j++) {
            uint32_t color = color_to_uint32(bg_color);
            uint8_t *pixel = row + j * 3;
            pixel[0] = color & 0xFF;
            pixel[1] = (color >> 8) & 0xFF;
            pixel[2] = (color >> 16) & 0xFF;
        }
    }
}

// void vbe_load_font(uint32_t font_start) {
//     psf1_header_t *header = (psf1_header_t *)font_start;
//     printf(" will load font ");
//     if (header->magic != 0x0436) {
//         printf(" wrong font magic:%08x %08x ", &header->magic, header->magic);
//         return;
//     }
//     uint8_t mode = header->mode;
//     char_height = header->charsize;
//     num_chars = (mode & 0x02) ? 512 : 256;
//     size_t bitmap_size = num_chars * char_height;
//     uint8_t *src = (uint8_t *)(font_start + sizeof(psf1_header_t));
//     memcpy(font_bitmap, src, bitmap_size);
//     max_col = mode_info->width / 8;
//     max_row = mode_info->height / char_height;
//     printf("\n font loaded: mode:0x%02x char_height:0x%02x num_chars:0x%02x bitmap_size:0x%02x max_col:0x%02x max_row:0x%02x ", mode, char_height, num_chars, bitmap_size,max_col, max_row);
// }
// void vbe_load_font(uint32_t font_start) {
//     psf2_header_t *header = (psf2_header_t *)font_start;
//     // printf("  ");
//     if (header->magic != 0x864ab572) {
//         return;
//     }
//     char_height = header->height;
//     char_width = header->width;
//     num_chars = header->numglyph;
//     size_t bitmap_size = num_chars * header->bytesperglyph;
//     uint8_t *src = (uint8_t *)(font_start + header->headersize);
//     memcpy(font_bitmap, src, bitmap_size);
//     max_col = mode_info->width / char_width;
//     max_row = mode_info->height / char_height;
//     printf("\n font loaded: font_start: %08x char_width:%02x char_height:%02x num_chars:%08x bitmap_size:%08x headersize:%08x version:%08x flags:%08x max_col:%d max_row:%d ", font_start, char_width, char_height, num_chars, bitmap_size, header->headersize, header->version, header->flags, max_col, max_row);

// }
void vbe_load_font(uint32_t font_start) {
    psf2_header_t *header = (psf2_header_t *)font_start;
    if (header->magic != 0x864ab572) {
        return;
    }
    char_height = header->height;
    char_width = header->width;
    num_chars = header->numglyph;
    bytes_per_glyph = header->bytesperglyph;
    size_t bitmap_size = num_chars * bytes_per_glyph;
    uint8_t *src = (uint8_t *)(font_start + header->headersize);
    memcpy(font_bitmap, src, bitmap_size);
    max_col = mode_info->width / char_width;
    max_row = mode_info->height / char_height;

    //     if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    //     print_vbe_mode_info(mode_info);
    // while(1);
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // printf("width:%d height:%d charwidth:%d charheight:%d max_row:%d max_col:%d", mode_info->width, mode_info->height, char_width, char_height, max_row, max_col);
    // print_vbe_mode_info(mode_info);    
    // while(1);


    if (header->flags & 1) {
        uint8_t *p = (uint8_t *)(font_start + header->headersize + header->numglyph * header->bytesperglyph);
        memset(unicode_to_glyph, 255, sizeof(unicode_to_glyph));
        for (int glyph = 0; glyph < header->numglyph; glyph++) {
            while (1) {
                if (*p == 0xFF) {
                    p++;
                    break;
                } else if (*p == 0xFE) {
                    p++;
                    while (*p != 0xFE && *p != 0xFF) {
                        int len = utf8_sequence_length(*p);
                        p += len;
                    }
                } else {
                    uint8_t *start = p;
                    uint32_t code_point = read_utf8(&p);
                    if (code_point < 65536 && unicode_to_glyph[code_point] == 255) {
                        unicode_to_glyph[code_point] = glyph;
                    }
                    if (p == start) {
                        p++; // invalid UTF-8
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < header->numglyph && i < 65536; i++) {
            unicode_to_glyph[i] = i;
        }
    }
}

static void vbe_update_prerendered_chars(void) {
    for (int glyph = 0; glyph < num_chars; glyph++) {
        uint8_t *char_bitmap = font_bitmap + glyph * bytes_per_glyph;
        for (int i = 0; i < char_height; i++) {
            uint8_t row = char_bitmap[i];
            for (int j = 0; j < char_width; j++) {
                int bit = (row >> (7 - j)) & 1;
                uint32_t color = bit ? color_to_uint32(fg_color) : color_to_uint32(bg_color);
                uint8_t *pixel = prerendered_chars[glyph].data[i] + j * 3;
                pixel[0] = color & 0xFF;
                pixel[1] = (color >> 8) & 0xFF;
                pixel[2] = (color >> 16) & 0xFF;
            }
        }
    }
}


void vbe_set_fg_color(uint8_t r, uint8_t g, uint8_t b) {
    fg_color.r = r;
    fg_color.g = g;
    fg_color.b = b;
    vbe_update_prerendered_chars();
}

void vbe_set_bg_color(uint8_t r, uint8_t g, uint8_t b) {
    bg_color.r = r;
    bg_color.g = g;
    bg_color.b = b;
    vbe_update_prerendered_chars();
}

void vbe_reset_textcolor(void) {
    fg_color = (color_t){255, 255, 255};
    bg_color = (color_t){0, 0, 0};
    vbe_update_prerendered_chars();
}

void vbe_putchar_at(char c, int x, int y) {
    draw_char_at(c, x, y);
}


void vbe_putchar(char c) {
    // if(vbe_set_text_mode()) printf("5error setting text mode err\n");
    // printf("printing:%c ", c);
    if (c == '\n') {
        cursor_col = 0;
        if (cursor_row < max_row - 1) {
            cursor_row++;
        } else {
            scroll_up();
            cursor_row = max_row - 1;
        }
    } else if (c == '\r') {
        cursor_col = 0;
    } else {
        int char_x = cursor_col * char_width;
        int char_y = cursor_row * char_height;
        // if(char_x>=1600){
        //     if(vbe_set_text_mode()) printf("5error setting text mode err\n");
        //     printf("char_x:%d char_y:%d cursor_col:%d cursor_row:%d char_width:%d char_height:%d max_row:%d max_col:%d", char_x, char_y, cursor_col, cursor_row, char_width, char_height, max_row, max_col);
        //     while(1);
        // }
        draw_char_at(c, char_x, char_y);
        cursor_col++;
        if (cursor_col>= max_col) {
            cursor_col = 0;
            if (cursor_row < max_row - 1) {
                cursor_row++;
            } else {
                scroll_up();
                cursor_row = max_row - 1;
            }
        }
    }
}
// void vbe_putchar(char c) {
//     if (c == '\n') {
//         cursor_col = 0;
//         if (cursor_row < max_row - 1) {
//             cursor_row++;
//         } else {
//             scroll_up();
//             cursor_row = max_row - 1;
//         }
//     } else if (c == '\r') {
//         cursor_col = 0;
//     } else {
//         int char_x = cursor_col * 8;
//         int char_y = cursor_row * char_height;
//         draw_char_at(c, char_x, char_y);
//         cursor_col++;
//         if (cursor_col >= max_col) {
//             cursor_col = 0;
//             if (cursor_row < max_row - 1) {
//                 cursor_row++;
//             } else {
//                 scroll_up();
//                 cursor_row = max_row - 1;
//             }
//         }
//     }
// }