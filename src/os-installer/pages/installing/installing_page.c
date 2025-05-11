#include "installing_page.h"
#include "../page.h"
#include <kernel/drivers/vga.h>
#include <stdio.h>
#include "../../utils/progress.h"
#include <kernel/drivers/timer.h>
// #define VGA_WIDTH 80
// #define VGA_HEIGHT 25

// extern disk_t disks[];
// extern uint8_t disks_count;
// extern size_t selected_disk;
// extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
// extern int is_free_space;

/* Wait for specified ticks */
void timer_wait(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while (timer_ticks - start < ticks) asm("hlt");
}

void install_os(void) {
    /* Placeholder: Simulate installation */
    for (uint32_t i = 0; i <= 100; i++) {
        update_progress(i);
        timer_wait(10); /* Simulate work */
    }
}

void installing_page(void) {
    vga_set_textcolor(normal_fg, normal_bg);
    vga_clear();
    vga_gotoxy(0, 0);
    printf("  Installing %s\n", OS_NAME);
    printf("  ---------------\n");
    printf("  Please wait while %s is installed on the selected partition.\n\n", OS_NAME);
    
    init_progress(100, 60);
    uint32_t start_ticks = timer_ticks;
    
    install_os();
    
    /* Display elapsed time */
    uint32_t elapsed = (timer_ticks - start_ticks) / 100; /* Seconds */
    vga_gotoxy(0, VGA_HEIGHT - 2);
    printf("  Installation complete in %d seconds.\n", elapsed);
    
    /* No actions; wait indefinitely */
    for(;;) asm("hlt");
}
