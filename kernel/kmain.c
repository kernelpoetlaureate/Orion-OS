#include <stdint.h>
#include <stddef.h>
#include "drivers/vga.h"
#include "core/log.h"
#include "drivers/serial.h"
#include "lib/include/libc.h"
#include "core/io.h"

extern char __git_shortsha[]; /* Optional: linker-provided string */

void kmain(void) {
    // Ensure serial is initialized before printing
    serial_init();

    // Lowest-level test: single characters
    serial_putc('H');
    serial_putc('i');
    serial_putc('\n');

    // Test printf with a simple message
    printf("printf test\n");

    // Manual byte-by-byte write using serial_putc with a small delay
    const char *msg = "[MANUAL] Hello from manual serial_putc\n";
    for (const char *p = msg; *p; ++p) {
        serial_putc(*p);
        // tiny delay to avoid flooding FIFO (simple busy loop)
        for (volatile int d = 0; d < 1000; ++d) { __asm__ volatile ("nop"); }
    }

    // Repeat single-character writes to test FIFO/timing
    for (int i = 0; i < 20; ++i) {
        serial_putc('A');
        // tiny delay
        for (volatile int d = 0; d < 10000; ++d) { __asm__ volatile ("nop"); }
    }
    serial_putc('\n');

    // Direct COM1 test using outb to bypass serial_putc/serial_write
    const char *raw = "[RAW] Direct outb COM1 test\n";
    const uint16_t COM1 = 0x3F8;
    for (const char *p = raw; *p; ++p) {
        // write directly to COM1 data port
        outb(COM1, (uint8_t)*p);
        // tiny delay
        for (volatile int d = 0; d < 10000; ++d) { __asm__ volatile ("nop"); }
    }

    for (;;) __asm__ volatile ("hlt"); // Simplify `kmain` to halt the CPU for debugging
}
