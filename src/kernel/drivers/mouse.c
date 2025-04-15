#include <kernel/drivers/mouse.h>
#include <kernel/isrs/mouse.h>
#include <kernel/mport.h>
#include <kernel/mconio.h>
#include <kernel/drivers/cursor.h> // For cursor updates


static struct mouse_data_t _mouse_data = {
    .buffer = {0},
    .cycle = 0,
    .x = DEFAULT_SCREEN_WIDTH / 2,
    .y = DEFAULT_SCREEN_HEIGHT / 2,
    .last_dx = 0.0f,
    .last_dy = 0.0f,
    .buttons = 0,
    .screen_width = DEFAULT_SCREEN_WIDTH,
    .screen_height = DEFAULT_SCREEN_HEIGHT,
    .speed = MOUSE_NORMAL_SPEED 
};

// Wait functions
static int mouse_wait_read(void) {
    int32_t timeout = MOUSE_TIMEOUT;
    while (!(inb(0x64) & 0x01) && timeout--) asm("nop");
    if (timeout == 0) { cprintf("Mouse wait_read timeout\n"); return 0; }
    return 1;
}

static int mouse_wait_write(void) {
    int32_t timeout = MOUSE_TIMEOUT;
    while ((inb(0x64) & 0x02) && timeout--) asm("nop");
    if (timeout == 0) { cprintf("Mouse wait_write timeout\n"); return 0; }
    return 1;
}

static void mouse_write(uint8_t value) {
    if (!mouse_wait_write()) return;
    outb(0x64, 0xD4);
    if (!mouse_wait_write()) return;
    outb(0x60, value);
}

static uint8_t mouse_read(void) {
    if (!mouse_wait_read()) return 0;
    return inb(0x60);
}

// ISR handler updates cursor
static void mouse_handle_data(uint8_t data) {
    _mouse_data.buffer[_mouse_data.cycle++] = data;

    if (_mouse_data.cycle == 3) {
        _mouse_data.cycle = 0;

        uint8_t flags = _mouse_data.buffer[0];
        if (!(flags & 0x08)) return;

        _mouse_data.buttons = flags & (MOUSE_LEFT_BUTTON | MOUSE_RIGHT_BUTTON | MOUSE_MIDDLE_BUTTON);

        int8_t raw_dx = _mouse_data.buffer[1];
        if (flags & 0x10) raw_dx |= 0xFFFFFF00;

        int8_t raw_dy = _mouse_data.buffer[2];
        if (flags & 0x20) raw_dy |= 0xFFFFFF00;
        raw_dy = -raw_dy;

        // Scaling with baselines
        float x_factor = ((float)_mouse_data.speed / MOUSE_NORMAL_SPEED) * 
        ((float)_mouse_data.screen_width / SCREEN_WIDTH_BASELINE);
        float y_factor = ((float)_mouse_data.speed / MOUSE_NORMAL_SPEED) * 
        ((float)_mouse_data.screen_height / SCREEN_HEIGHT_BASELINE);

        _mouse_data.last_dx = (float)raw_dx * x_factor;
        _mouse_data.last_dy = (float)raw_dy * y_factor;

        _mouse_data.x += (float)_mouse_data.last_dx;
        _mouse_data.y += (float)_mouse_data.last_dy;

        // Clamp to screen bounds
        if (_mouse_data.x < 0) _mouse_data.x = 0;
        if (_mouse_data.y < 0) _mouse_data.y = 0;
        if (_mouse_data.screen_width > 0 && _mouse_data.x >= _mouse_data.screen_width) 
        _mouse_data.x = _mouse_data.screen_width - 1;
        if (_mouse_data.screen_height > 0 && _mouse_data.y >= _mouse_data.screen_height) 
        _mouse_data.y = _mouse_data.screen_height - 1;
        cursor_drv.move(_mouse_data.x, _mouse_data.y); // ISR updates cursor
    }
}

static void mouse_init(int32_t width, int32_t height) {
    if (width < MIN_SCREEN_SIZE || height < MIN_SCREEN_SIZE) {
        width = DEFAULT_SCREEN_WIDTH;
        height = DEFAULT_SCREEN_HEIGHT;
    }

    mouse_wait_write(); outb(0x64, 0xA8);
    mouse_wait_write(); outb(0x64, 0x20);
    uint8_t status = mouse_read();
    status |= 0x02; status &= ~0x20;
    mouse_wait_write(); outb(0x64, 0x60);
    mouse_wait_write(); outb(0x60, status);
    mouse_write(0xF6); mouse_read();
    mouse_write(0xF4); mouse_read();

    isr_mouse_register_handler(mouse_handle_data);

    _mouse_data.screen_width = width;
    _mouse_data.screen_height = height;
    _mouse_data.x = width / 2;
    _mouse_data.y = height / 2;
    _mouse_data.buttons = 0;
    _mouse_data.cycle = 0;
    _mouse_data.speed = MOUSE_NORMAL_SPEED * 2;
    _mouse_data.last_dx = 0.0f;
    _mouse_data.last_dy = 0.0f;

    cursor_drv.init(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT); // Setup cursor
    cursor_drv.show(); // Show cursor
}

// Rest unchanged
static int32_t mouse_get_x(void) { return _mouse_data.x; }
static int32_t mouse_get_y(void) { return _mouse_data.y; }
static uint8_t mouse_get_buttons(void) { return _mouse_data.buttons; }
static void mouse_set_screen_size(int32_t width, int32_t height) {
    if (width < MIN_SCREEN_SIZE || height < MIN_SCREEN_SIZE) {
        width = DEFAULT_SCREEN_WIDTH;
        height = DEFAULT_SCREEN_HEIGHT;
    }
    _mouse_data.screen_width = width;
    _mouse_data.screen_height = height;
    _mouse_data.x = width / 2;
    _mouse_data.y = height / 2;
    cursor_drv.move(_mouse_data.x, _mouse_data.y);
}
static void mouse_set_speed(int32_t speed) { if (speed > 0) _mouse_data.speed = speed; }
static int32_t mouse_get_dx(void) { return (int32_t)_mouse_data.last_dx; }
static int32_t mouse_get_dy(void) { return (int32_t)_mouse_data.last_dy; }
static void mouse_enable(void) { mouse_write(0xF4); mouse_read(); }
static void mouse_disable(void) { mouse_write(0xF5); mouse_read(); }
static void mouse_set_resolution(uint8_t resolution) {
    if (resolution > 3) return;
    mouse_write(0xE8); mouse_read();
    mouse_write(resolution); mouse_read();
}
static void mouse_set_sample_rate(uint8_t rate) {
    if (rate < 10 || rate > 200) return;
    mouse_write(0xF3); mouse_read();
    mouse_write(rate); mouse_read();
}

struct mouse_driver mouse_drv = {
    .init = mouse_init,
    .get_x = mouse_get_x,
    .get_y = mouse_get_y,
    .get_buttons = mouse_get_buttons,
    .set_screen_size = mouse_set_screen_size,
    .set_speed = mouse_set_speed,
    .get_dx = mouse_get_dx,
    .get_dy = mouse_get_dy,
    .enable_mouse = mouse_enable,
    .disable_mouse = mouse_disable,
    .set_resolution = mouse_set_resolution,
    .set_sample_rate = mouse_set_sample_rate
};