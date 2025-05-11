#include "welcome_page.h"
#include "../page.h"
#include <kernel/drivers/vga.h>
#include <stdio.h>
#include "../../utils/reboot.h"
#include "../tos/tos_page.h"

// extern disk_t disks[];
// extern uint8_t disks_count;
// extern size_t selected_disk;
// extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
// extern int is_free_space;

static void proceed_action(void) {
    tos_page();
}

static void reboot_action(void) {
    reboot();
}

static action_t actions[] = {
    {"Enter", KEY_ENTER, "Proceed", proceed_action},
    {"Esc", KEY_ESC, "Reboot", reboot_action}
};

void welcome_page(void) {
    vga_set_textcolor(normal_fg, normal_bg);
    vga_clear();
    vga_gotoxy(0, 0);
    printf("  Welcome to the %s Installation Wizard\n", OS_NAME);
    printf("  --------------------------------------\n");
    printf("  Thank you for choosing %s, a free and open-source operating system.\n", OS_NAME);
    printf("  This wizard will guide you through the installation process.\n\n");
    printf("  Press Enter to begin or Esc to reboot your computer.\n");
    
    display_action_bar(actions, sizeof(actions) / sizeof(actions[0]));
    handle_input(actions, sizeof(actions) / sizeof(actions[0]));
}
