#include "create_partition_page.h"
#include "../page.h"
#include <kernel/drivers/vga.h>
#include <stdio.h>
#include "../setup/setup_page.h"


// extern static size_t selected_disk;

// extern disk_t disks[];
// extern uint8_t disks_count;
// extern size_t selected_disk;
// extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
// extern int is_free_space;

int size_mb;

static void create_action(void) {
        

    /* Placeholder: Create partition logic */
    // setup_page();
}

static void cancel_action(void) {
    setup_page();
}

static action_t actions[] = {
    {"Enter", KEY_ENTER, "Create", create_action},
    {"Esc", KEY_ESC, "Cancel", cancel_action}
};

void create_partition_page(void) {
    while(1){
        vga_set_textcolor(normal_fg, normal_bg);
        vga_clear();
        display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
        vga_gotoxy(0, 0);
        printf("  Create a New Partition\n");
        printf("  ---------------------\n");
        printf("  Selected Device: Disk %d\n", selected_disk);
        printf("  Free Space: %dMB\n\n", disks[selected_disk].free_space_mb);
        printf("  Enter the size (in MB) for the new partition (max %dMB).\n", disks[selected_disk].free_space_mb);
        printf("  The partition will be created on the selected device.\n\n");
        printf("  Size (MB): ");
        scanf("%d", &size_mb);
        if(size_mb<=disks[selected_disk].free_space_mb){
            break;
        }
    }

    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}
