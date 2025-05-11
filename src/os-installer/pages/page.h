#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/drivers/vga.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/keyboard.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef enum keycode keycode_t;

typedef struct {
    const char* keyname;
    keycode_t keycode;
    const char* actionname;
    void (*action)(void);
} action_t;

typedef struct {
    uint32_t size_mb; /* Size in MB */
    const char* format; /* e.g., "FAT32" or "Unformatted" */
} partition_t;

typedef struct {
    uint32_t total_size_mb;
    partition_t* partitions;
    size_t num_partitions;
    uint32_t free_space_mb;
} disk_t;


extern uint32_t timer_ticks;
extern const char* OS_NAME; /* Global OS name */

extern disk_t disks[];
extern uint8_t disks_count;
extern size_t selected_disk;
extern size_t selected_item; /* 0 to num_partitions for partitions, num_partitions for free space */
extern int is_free_space;

extern const char* OS_NAME;

// static uint8_t disks_count;
// static size_t selected_disk = 0;
// static size_t selected_item = 0; /* 0 to num_partitions for partitions, num_partitions for free space */
// static int is_free_space = 0;


static uint8_t actionbar_fg = VGA_COLOR_BLACK;
static uint8_t actionbar_bg = VGA_COLOR_LIGHT_GREY;
static uint8_t normal_fg = VGA_COLOR_BLACK;
static uint8_t normal_bg = VGA_COLOR_WHITE;
static uint8_t hovered_fg = VGA_COLOR_WHITE;
static uint8_t hovered_bg = VGA_COLOR_BLACK;


void run_os_installer_wisard();
void display_action_bar(action_t* actions, size_t num_actions);
void handle_input(action_t* actions, size_t num_actions);

#endif
