#ifndef _TIME_H
#define _TIME_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long time_t;
typedef long clock_t;
struct timespec { time_t tv_sec; long tv_nsec; };
struct tm {
    int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;
};
time_t time(time_t *t);
int nanosleep(const struct timespec *req, struct timespec *rem);
char *ctime(const time_t *timep);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
time_t mktime(struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif
