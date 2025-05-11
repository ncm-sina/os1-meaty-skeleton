#include "setup_page.h"
#include "../page.h"
#include <kernel/drivers/vga.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include "../../utils/reboot.h"
#include "../create_partition/create_partition_page.h"
#include "../delete_partition/delete_partition_page.h"
#include "../format_partition/format_partition_page.h"
#include "../installing/installing_page.h"

// extern disk_t disks[];
// extern uint8_t disks_count;
// extern size_t selected_disk;
// extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
// extern int is_free_space;


// typedef struct {
//     uint32_t size_mb; /* Size in MB */
//     const char* format; /* e.g., "FAT32" or "Unformatted" */
// } partition_t;

// typedef struct {
//     uint32_t total_size_mb;
//     partition_t* partitions;
//     size_t num_partitions;
//     uint32_t free_space_mb;
// } disk_t;

// static disk_t disks[] = {
//     {1024, (partition_t[]){{500, "FAT32"}, {300, "Unformatted"}}, 2, 224},
//     {2048, NULL, 0, 2048}
// };

// static size_t selected_disk = 0;
// static size_t selected_item = 0; /* 0 to num_partitions for partitions, num_partitions for free space */
// static int is_free_space = 0;

static void install_action(void) {
    installing_page();
}

static void create_partition_action(void) {
    create_partition_page();
}

static void delete_partition_action(void) {
    delete_partition_page();
}

static void format_partition_action(void) {
    format_partition_page();
}

static void quit_action(void) {
    reboot();
}

static action_t partition_actions[] = {
    {"Enter", KEY_ENTER, "Install", install_action},
    {"D", KEY_D, "Delete Partition", delete_partition_action},
    {"F", KEY_F, "Format Partition", format_partition_action},
    {"F3", KEY_F3, "Quit", quit_action}
};

static action_t free_space_actions[] = {
    {"Enter", KEY_ENTER, "Install", install_action},
    {"C", KEY_C, "Create Partition", create_partition_action},
    {"F3", KEY_F3, "Quit", quit_action}
};

static void display_disks(void) {
    vga_gotoxy(0, 3);
    // size_t line = 3;
    // size_t item_idx = 0;
    for (int i = 0; i < disks_count; i++) {
        printf("  Disk %d (%dMB):\n", i, disks[i].total_size_mb);
        // line++;
        int j=0;
        
        for (j = 0; j < disks[i].num_partitions; j++) {
            if (i == selected_disk && j == selected_item && !is_free_space) {
                vga_set_textcolor(hovered_fg, hovered_bg);
            } else {
                vga_set_textcolor(normal_fg, normal_bg);
            }
            printf("    - Partition %d: %dMB (%s)\n", j + 1, disks[i].partitions[j].size_mb,
                   disks[i].partitions[j].format);
            vga_set_textcolor(normal_fg, normal_bg);
                   // line++;
            // item_idx++;
        }
        
        if (disks[i].free_space_mb > 0) {
            if (i == selected_disk && j == selected_item && is_free_space) {
                vga_set_textcolor(hovered_fg, hovered_bg);
            } else {
                vga_set_textcolor(normal_fg, normal_bg);
            }
            printf("    - Free Space: %dMB\n", disks[i].free_space_mb);
            vga_set_textcolor(normal_fg, normal_bg);
            // line++;
            // item_idx++;
        }
    }
}

static inline get_max_items(int disk_i){
    return disks[disk_i].num_partitions + (disks[disk_i].free_space_mb > 0 ? 1 : 0);
}

void setup_page(void) {
    while (1) {
        vga_set_textcolor(normal_fg, normal_bg);
        vga_clear();
        vga_gotoxy(0, 0);
        printf("  Select a Partition for %s Installation\n", OS_NAME);
        printf("  ---------------------------------------\n");
        printf("  Please select a storage device and partition to install %s.\n", OS_NAME);
        
        display_disks();
        
        action_t* actions = is_free_space ? free_space_actions : partition_actions;
        size_t num_actions = is_free_space ? sizeof(free_space_actions) / sizeof(free_space_actions[0])
                                          : sizeof(partition_actions) / sizeof(partition_actions[0]);
        display_action_bar(actions, num_actions);

        // uint32_t last_tick = timer_drv.get_ticks();    
        while (1) {
            // uint32_t current_tick = timer_drv.get_ticks();
            struct key_event event;            
            if (keyboard_drv.get_event(&event)) {
                // enum keycode code = event.code;
                // printf("  \npressed keycode: %04x, type:%d ", event.code, event.type);
                // key_pressed = 0;
                
                if (event.code == KEY_UP && event.type == KEY_PRESS) {
                    keyboard_drv.clear_buffer();
                    if (selected_item > 0) {
                        selected_item--;
                    }else if(selected_disk>0){
                        selected_disk--;
                        selected_item=get_max_items(selected_disk)-1;
                    }
                    
                    if (selected_item < disks[selected_disk].num_partitions) {
                        is_free_space = 0;
                    } else {
                        is_free_space = 1;
                    }
                    break;
                } else if (event.code == KEY_DOWN && event.type == KEY_PRESS) {
                    keyboard_drv.clear_buffer();
                    size_t max_item =get_max_items(selected_disk);
                    if (selected_item + 1 < max_item) {
                        selected_item++;
                    }else if(selected_disk<disks_count-1){
                        selected_item=0;
                        selected_disk++;
                    }
                    
                    if (selected_item >= disks[selected_disk].num_partitions) {
                        is_free_space = 1;
                    } else {
                        is_free_space = 0;
                    }
                    break;                
                } else {
                    for (size_t i = 0; i < num_actions; i++) {
                        if (event.code == actions[i].keycode) {
                            actions[i].action();
                            return;
                        }
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
}
