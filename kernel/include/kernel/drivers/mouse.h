#ifndef DRIVERS_MOUSE_H
#define DRIVERS_MOUSE_H

#include <stdint.h>

#define MOUSE_LEFT_BUTTON   (1 << 0)
#define MOUSE_RIGHT_BUTTON  (1 << 1)
#define MOUSE_MIDDLE_BUTTON (1 << 2)
#define MOUSE_NORMAL_SPEED  10
#define DEFAULT_SCREEN_WIDTH  80   // Text mode default
#define DEFAULT_SCREEN_HEIGHT 25   // Text mode default
#define MIN_SCREEN_SIZE       10
#define SCREEN_WIDTH_BASELINE 102 // Baseline for scaling
#define SCREEN_HEIGHT_BASELINE 72 // Baseline for scaling

#define MOUSE_BUFFER_SIZE 3 // Standard PS/2 mouse sends 3-byte packets
#define MOUSE_TIMEOUT 20000000 // Timeout for wait functions (~1ms at 1GHz)

typedef struct mouse_data_t {
    int8_t buffer[MOUSE_BUFFER_SIZE];
    int8_t cycle;          // Tracks packet byte position (0-2)
    int32_t x;              // Absolute X position
    int32_t y;              // Absolute Y position
    float last_dx;          // Last X delta (float for precision)
    float last_dy;          // Last Y delta (float for precision)
    int8_t buttons;        // Button states
    int32_t screen_width;  // Screen width in pixels/characters
    int32_t screen_height; // Screen height in pixels/characters
    int32_t speed;         // Speed multiplier
} mouse_data_t;

struct mouse_driver {
    void (*init)(int32_t screen_width, int32_t screen_height);
    int32_t (*get_x)(void);
    int32_t (*get_y)(void);
    int8_t (*get_buttons)(void);
    void (*set_screen_size)(int32_t screen_width, int32_t screen_height);
    void (*set_speed)(int32_t speed);
    int32_t (*get_dx)(void);
    int32_t (*get_dy)(void);
    void (*enable_mouse)(void);           // Enable mouse data reporting
    void (*disable_mouse)(void);          // Disable mouse data reporting
    void (*set_resolution)(int8_t resolution); // Set resolution (0-3)
    void (*set_sample_rate)(int8_t rate);      // Set sample rate (10-200)
};

extern struct mouse_driver mouse_drv;

#endif