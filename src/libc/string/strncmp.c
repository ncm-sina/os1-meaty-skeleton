#include <string.h>

int strncmp(const char *s1, const char *s2, size_t n) {
    if(n<1)
        return 0;
    while (n>0 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
