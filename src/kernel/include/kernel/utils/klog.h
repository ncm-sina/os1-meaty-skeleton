#ifndef KLOG_H
#define KLOG_H

#include <stdio.h>

typedef struct{
    void (*error)(const char *msg);
    void (*success)(const char *msg);
} logger_t;

// logger_t *logger;
extern logger_t *logger;

int logger_init();

void logger_error(const char *msg);

void logger_success(const char *msg);

void klog(const char *msg);


#endif