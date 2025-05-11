#include "format_partition_page.h"
#include "../page.h"
#include <kernel/drivers/vga.h>
#include <stdio.h>
#include "../setup/setup_page.h"

// extern disk_t disks[];
// extern uint8_t disks_count;
// extern size_t selected_disk;
// extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
// extern int is_free_space;


static void format_action(void) {
    /* Placeholder: Format partition logic */
    setup_page();
}

static void cancel_action(void) {
    setup_page();
}

static action_t actions[] = {
    {"F", KEY_F, "Format", format_action},
    {"Esc", KEY_ESC, "Cancel", cancel_action}
};

void format_partition_page(void) {
    vga_set_textcolor(normal_fg, normal_bg);
    vga_clear();
    vga_gotoxy(0, 0);
    printf("  Format Partition\n");
    printf("  ----------------\n");
    printf("  Selected Partition: Disk %d, Partition %d (%dMB)\n",
           selected_disk, selected_item + 1, disks[selected_disk].partitions[selected_item].size_mb);
    printf("  \nWARNING: Formatting this partition as FAT32 will erase all data.\n");
    printf("  This action cannot be undone. Ensure you have backups.\n\n");
    printf("  Confirm formatting or cancel to return.\n");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}
