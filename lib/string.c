#include "string.h"

void *memset(void *dst, int c, uint64_t n)
{
    char *cdst = (char *)dst;
    for (uint64_t i = 0; i < n; ++i)
        cdst[i] = c;

    return dst;
}

void memcpy(void *target, void *source, int size)
{
    char *xtarget = (char *)target;
    char *xsource = (char *)source;
    for (int i = 0; i < size; i++)
    {
        xtarget[i] = xsource[i];
    }
}
