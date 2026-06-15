/*
* ----------------------------------------------------------------------------
* TangPds (c) Jason Wilden 2026
* ----------------------------------------------------------------------------
*/
#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dest;
}

