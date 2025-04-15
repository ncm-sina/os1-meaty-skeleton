#include <utils/klog.h>
#include <libk/stdio.h>

void klog(const char *msg) {
    kprintf("[LOG] %s\n", msg);
}