#include <stdint.h>
#include <stddef.h>
#include "kernel/drivers/vga.h"
#include "kernel/core/log.h"

extern char __git_shortsha[]; /* Optional: linker-provided string */

void kmain(void) {
    vga_init();
    serial_init();
    kprintf("Kernel v0.1 (git %s) online\n", "<shortsha>");
    vga_write("Kernel v0.1 (git <shortsha>) online\n");
    for (;;) __asm__ volatile ("hlt");
}
