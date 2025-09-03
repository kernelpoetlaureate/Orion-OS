#include <stdint.h>
#include <stddef.h>
#include "drivers/vga.h"
#include "core/log.h"
#include "drivers/serial.h"

extern char __git_shortsha[]; /* Optional: linker-provided string */

void kmain(void) {
    for (;;) __asm__ volatile ("hlt"); // Simplify `kmain` to halt the CPU for debugging
}
