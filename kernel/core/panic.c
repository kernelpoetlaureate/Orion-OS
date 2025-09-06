#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "../drivers/serial.h"
#include "../lib/include/libc.h"

/* vsnprintf is provided by kernel/lib/printf.c */
int vsnprintf(char *out, size_t size, const char *fmt, va_list ap);

void panic(const char *fmt, ...) {
    /* Ensure serial is initialized */
    serial_init();

    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* Write the formatted panic message and newline to serial */
    serial_write(buf);
    serial_write("\n");

    /* Halt the CPU */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
