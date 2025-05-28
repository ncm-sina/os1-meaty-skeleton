#include <kernel/utils/klog.h>

logger_t* logger = NULL;

int logger_init(){
    static logger_t logger_instance; // Static instance to hold the struct
    logger = &logger_instance;       // Point global logger to this instance
    logger->error = logger_error;
    logger->success = logger_success;
    return 0;
}

void logger_error(const char *msg){
    unsigned char attr = vga_get_textcolor();
    vga_set_textcolor(VGA_COLOR_RED, VGA_COLOR_BLACK);
    printf("%s\n", msg);
    // vga_set_textcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_set_textcolor(attr & 0x0F, (attr >> 4) & 0xF );
}

void logger_success(const char *msg){
    unsigned char attr = vga_get_textcolor();
    vga_set_textcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("%s\n", msg);
    // vga_set_textcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_set_textcolor(attr & 0x0F, (attr >> 4) & 0xF );
}

void klog(const char *msg) {
    printf("[LOG] %s\n", msg);
}
