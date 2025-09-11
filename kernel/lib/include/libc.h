#ifndef ORION_LIBC_H
#define ORION_LIBC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);
int strcmp(const char *a, const char *b);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* ORION_LIBC_H */
