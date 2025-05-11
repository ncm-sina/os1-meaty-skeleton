#include "tos_page.h"
#include "../page.h"
#include <kernel/drivers/vga.h>
#include <stdio.h>
#include "../../utils/reboot.h"
#include "../setup/setup_page.h"

// extern disk_t disks[];
// extern uint8_t disks_count;
// extern size_t selected_disk;
// extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
// extern int is_free_space;

static void agree_action(void) {
    setup_page();
}

static void disagree_action(void) {
    reboot();
}

static action_t actions[] = {
    {"F8", KEY_F8, " Agree", agree_action},
    {"Esc", KEY_ESC, "Not Agree", disagree_action}
};

void tos_page(void) {
    vga_set_textcolor(normal_fg, normal_bg);
    vga_clear();
    vga_gotoxy(0, 0);
    printf("  %s Terms of Service\n", OS_NAME);
    printf("  ---------------------\n");
    printf("  %s is free software distributed under an open-source license.\n", OS_NAME);
    printf("  It comes with no warranty, express or implied. By installing %s,\n", OS_NAME);
    printf("  you agree to use it for lawful purposes only and accept all risks\n");
    printf("  associated with its use.\n\n");
    printf("  Please read and agree to proceed with the installation.\n");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}
