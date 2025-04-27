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
// #endif

#define _AX 0x6000
#define _BX 0x6002
#define _CX 0x6004
#define _DX 0x6006
#define _DI 0x6008
#define _ES 0x600A

uint16_t *vbe_bc_ax=_AX;
uint16_t *vbe_bc_bx=_BX;
uint16_t *vbe_bc_cx=_CX;
uint16_t *vbe_bc_dx=_DX;
uint16_t *vbe_bc_di=_DI;
uint16_t *vbe_bc_es=_ES;

extern uint32_t *_vbe_bc_text_start;
extern uint32_t *_vbe_bc_text_end;

extern uint32_t *_vbe_bc_data_start;
extern uint32_t *_vbe_bc_data_end;


// Global pointers
VbeInfoBlock* vbe_info = (VbeInfoBlock*) VBE_INFO;
ModeInfoBlock* mode_info = (ModeInfoBlock*) VBE_MINFO;
// uint32_t* framebuffer = (uint32_t*) FRAMEBUFFER_VADDR;
static uint64_t framebuffer_addr; // framebuffer physical address or dynamic vaddr (we should map this to a fixed address later)

// External page table mapping function (you must implement)
// extern void map_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

#ifndef VBE_RUN_SIZE 
#define VBE_RUN_SIZE 0x100
#endif

uint8_t vbe_run_bin[VBE_RUN_SIZE] = {
    0xa1, 0x0a, 0x60, 0x89, 0xc6, 0xa1, 0x00, 0x60, 
    0x8b, 0x1e, 0x02, 0x60, 0x8b, 0x0e, 0x04, 0x60, 
    0x8b, 0x16, 0x06, 0x60, 0x8b, 0x3e, 0x08, 0x60, 
    0xcd, 0x10, 0xc3
};


// Initialize VBE and check support
uint32_t vbe_init(void) {
    // Clear vbe_info and set signature
    for (int i = 0; i < sizeof(VbeInfoBlock); i++) {
        ((uint8_t*)vbe_info)[i] = 0;
    }
    vbe_info->signature[0] = 'V';
    vbe_info->signature[1] = 'E';
    vbe_info->signature[2] = 'S';
    vbe_info->signature[3] = 'A';

    struct bios_call bc = {
        .eax = 0x4F00,
        .es = (VBE_INFO >> 16) << 12 ,
        .di = VBE_INFO & 0xFFFF
    };

    // Call INT 0x10, AX=0x4F00 to get VBE info
    int res = vbe_bios_call(&bc);
    if (bc.eax != 0x004F) return -1;


    if (result != 0x004F || vbe_info->version < 0x0200) {
        return -2; // VBE not supported or version < 2.0
    }

    if (strncmp(vib->signature, "VESA", 4) != 0) return -1;

    return 0;
}

uint32_t vbe_bios_call(vbe_bios_call_t *bc){
    *vbe_bc_ax=bc.ax;
    *vbe_bc_bx=bc.bx;
    *vbe_bc_cx=bv.cx;
    *vbe_bc_dx=bc.dx;
    *vbe_bc_es=bc.es;
    *vbe_bc_di=bc.di;
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

    return call_rm_program(bin_info);
}

// Set a video mode
uint32_t vbe_set_mode(uint16_t width, uint16_t height, uint8_t bpp) {
    // Get mode list from vbe_info
    uint16_t* mode_list = (uint16_t*)(vbe_info->video_mode_ptr & 0xFFFF);
    uint16_t target_mode = 0;

    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        struct bios_call bc = {
            .eax = 0x4F01,
            .ecx = mode_list[i],
            .es = (VBE_MINFO >> 16) << 12 ,
            .di = VBE_MINFO & 0xFFFF
        };        
        if (vbe_bios_call(&bc) != 0x004F) continue;

        if (info.width == width && info.height == height && info.bpp == bpp &&
            (info.attributes & 0x80)) { // Linear framebuffer supported
            target_mode = modes[i] | 0x4000; // Enable LFB
            break;
        }
    }

    if (target_mode == 0) return -1;

    struct bios_call bc = {
        .eax = 0x4F02,
        .ebx = target_mode
    };
    int res = vbe_bios_call(&bc);
    if (bc.eax != 0x004F) return -1;

    // Get mode info to verify framebuffer
    bc.eax = 0x4F01;
    bc.ecx = target_mode & 0x1FFF; // Clear LFB bit
    bc.es = (VBE_MINFO >> 16) << 12 ,
    bc.di = VBE_MINFO & 0xFFFF
    if (vbe_bios_call(&bc) != 0x004F) return -1;

    if (!(_mbi->flags & (1 << 12))) return -1;
    framebuffer_addr = _mbi->framebuffer_addr; // Physical address

    // TODO: Map to 0xD0000000 in page tables
    // map_page(0xD0000000, info.framebuffer, 0x3);
    // TODO: Map to 0xD0000000 in page tables
    // map_page(0xD0000000, info.framebuffer, 0x3);
    return 0;
}

// Get mode information
uint32_t vbe_get_mode_info(uint16_t mode, ModeInfoBlock* info) {
    struct bios_call bc = {
        .eax = 0x4F01,
        .ecx = mode & 0x1FFF, // Clear LFB bit
        .es = VBE_MINFO >> 4,
        .di = VBE_MINFO & 0xFFFF
    };
    int res = vbe_bios_call(&bc);
    if (bc.eax != 0x004F) return -1;
    memcpy(info, mode_info, sizeof(ModeInfoBlock));
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
void vbe_clear_screen(uint32_t color) {
    if (!(_mbi->flags & (1 << 12))) return;
    for (uint32_t i = 0; i < mode_info->width * mode_info->height; i++) {
        framebuffer[i] = color;
    }
}

// Draw a pixel
void vbe_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (x < mode_info->width && y < mode_info->height) {
        uint32_t offset = y * (mode_info->pitch / 4) + x;
        framebuffer[offset] = color;
    }
}

// Fill a rectangle
void vbe_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    for (uint16_t j = y; j < y + height && j < mode_info->height; j++) {
        for (uint16_t i = x; i < x + width && i < mode_info->width; i++) {
            uint32_t offset = j * (mode_info->pitch / 4) + i;
            framebuffer[offset] = color;
        }
    }
}