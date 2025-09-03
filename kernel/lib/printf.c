#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "include/libc.h"
#include "../drivers/serial.h"
#include "../drivers/vga.h"

static void reverse(char *start, char *end) {
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
}

static char *utoa(unsigned long val, char *buf, int base, int lowercase) {
    char *p = buf;
    const char *digits = lowercase ? "0123456789abcdef" : "0123456789ABCDEF";
    if (val == 0) {
        *p++ = '0';
        *p = '\0';
        return buf;
    }
    while (val) {
        *p++ = digits[val % base];
        val /= base;
    }
    *p = '\0';
    reverse(buf, p - 1);
    return buf;
}

int vsnprintf(char *out, size_t size, const char *fmt, va_list ap) {
    char *start = out;
    size_t left = size ? size - 1 : 0; // reserve space for NUL

    while (*fmt) {
        if (*fmt != '%') {
            if (left) { *out++ = *fmt; --left; }
            ++fmt;
            continue;
        }
        ++fmt; // skip '%'
        int longflag = 0;
        if (*fmt == 'l') { longflag = 1; ++fmt; }
        char buf[32];
        switch (*fmt++) {
            case 'c': {
                char c = (char)va_arg(ap, int);
                if (left) { *out++ = c; --left; }
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                while (*s) {
                    if (left) { *out++ = *s; --left; }
                    ++s;
                }
                break;
            }
            case 'd': {
                long val = longflag ? va_arg(ap, long) : va_arg(ap, int);
                if (val < 0) { if (left) { *out++ = '-'; --left; } val = -val; }
                utoa((unsigned long)val, buf, 10, 0);
                char *p = buf;
                while (*p) { if (left) { *out++ = *p++; --left; } else { ++p; } }
                break;
            }
            case 'u': {
                unsigned long val = longflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                utoa(val, buf, 10, 0);
                char *p = buf;
                while (*p) { if (left) { *out++ = *p++; --left; } else { ++p; } }
                break;
            }
            case 'x': {
                unsigned long val = longflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                utoa(val, buf, 16, 1);
                char *p = buf;
                while (*p) { if (left) { *out++ = *p++; --left; } else { ++p; } }
                break;
            }
            case 'p': {
                void *ptr = va_arg(ap, void *);
                unsigned long val = (unsigned long)ptr;
                utoa(val, buf, 16, 1);
                char *p = buf;
                if (left) { *out++ = '0'; --left; }
                if (left) { *out++ = 'x'; --left; }
                while (*p) { if (left) { *out++ = *p++; --left; } else { ++p; } }
                break;
            }
            case '%': {
                if (left) { *out++ = '%'; --left; }
                break;
            }
            default:
                // Unknown specifier; ignore
                break;
        }
    }
    *out = '\0';
    return out - start;
}

int snprintf(char *out, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(out, size, fmt, ap);
    va_end(ap);
    return r;
}

void kprintf(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    // Ensure serial is up
    serial_init();
    serial_write(buf);
}

int printf(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serial_write(buf); // Output the formatted string to the serial port
    return ret; // Return the number of characters written
}
