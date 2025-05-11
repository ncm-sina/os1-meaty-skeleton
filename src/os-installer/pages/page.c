#include "page.h"
#include <stdio.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

const char* OS_NAME = "MyOS"; /* Global OS name */

uint8_t disks_count;
size_t selected_disk = 0;
size_t selected_item = 0; /* 0 to num_partitions for partitions, num_partitions for free space */
int is_free_space = 0;

disk_t disks[] = {
    {1024, (partition_t[]){{500, "FAT32"}, {300, "Unformatted"}}, 2, 224},
    {2048, NULL, 0, 2048}
};


void run_os_installer_wisard(){
    disks_count = sizeof(disks) / sizeof(disk_t);
    if(disks_count>0)
        selected_disk=0;
    else
        selected_disk=-1;

    
    // printf("  selected_disk:%d disk_count:%d ", selected_disk, disks_count);while(1);
    welcome_page();
}

void display_action_bar(action_t* actions, size_t num_actions) {
    vga_set_textcolor(actionbar_fg, actionbar_bg);
    vga_gotoxy(0, VGA_HEIGHT - 1);
    int i;
    for (i = 0; i < VGA_WIDTH-1; i++)
        vga_put_char(' ');
    vga_put_char_at(i,VGA_HEIGHT - 1,' ', actionbar_fg<<4 + actionbar_bg);

    vga_gotoxy(2, VGA_HEIGHT - 1);
    for (i = 0; i < num_actions; i++) {
        printf("  %s=%s  ", actions[i].keyname, actions[i].actionname);
    }
    vga_set_textcolor(normal_fg, normal_bg);
}

void handle_input(action_t* actions, size_t num_actions) {
    // vga_gotoxy(2,5);
    // printf("  \nhandle input:%d", num_actions);
    // const char* keyname;
    // keycode_t keycode;
    // const char* actionname;
    // void (*action)(void);

    // for(int i=0; i<num_actions; i++){
    //     printf("  \nkeyname: %s, keycode: %04x, actionname: %s, action:%08x", actions[i].keyname, actions[i].keycode, actions[i].actionname, actions[i].action );
    // }
    // uint32_t last_tick = timer_drv.get_ticks();
    while (1) {
        // uint32_t current_tick = timer_drv.get_ticks();
        struct key_event event;            
        if (keyboard_drv.get_event(&event)) {
            // enum keycode code = event.code;
            // printf("  \npressed keycode: %04x, type: ", event.code, event.type);
            // key_pressed = 0;
            for (size_t i = 0; i < num_actions; i++) {
                // printf("  action keycode: %04x ", actions[i].keycode);
                if (event.code == actions[i].keycode && event.type == KEY_PRESS) {
                    keyboard_drv.clear_buffer();
                    actions[i].action();
                    return;
                }
            }
        }
        // if (current_tick != last_tick) {
        //     last_tick = current_tick;
        //     vga_gotoxy(0, 0);
        //     printf("  Ticks: %08X", current_tick);
        // }
        // asm("hlt");
    }
}
