#include <stdint.h>
#include <stddef.h>
#include "drivers/vga.h"
#include "core/log.h"
#include "drivers/serial.h"

extern char __git_shortsha[]; /* Optional: linker-provided string */

void kmain(void) {
    vga_init();
    serial_init();
    kprintf("Kernel v0.1 (git %s) online\n", "<shortsha>");
    vga_write("Kernel v0.1 (git <shortsha>) online\n");
    for (;;) __asm__ volatile ("hlt");
}
