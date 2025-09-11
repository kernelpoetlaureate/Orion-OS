#include "libc.h"

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

size_t strnlen(const char *s, size_t maxlen) {
    const char *p = s;
    size_t i = 0;
    while (i < maxlen && *p) { ++p; ++i; }
    return i;
}
