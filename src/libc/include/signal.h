#ifndef _SIGNAL_H
#define _SIGNAL_H

// #include <sys/cdefs.h>

// #include <stddef.h>

#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

struct sigaction {
    void (*sa_handler)(int);
    int sa_flags;
};

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

#ifdef __cplusplus
}
#endif

#endif
