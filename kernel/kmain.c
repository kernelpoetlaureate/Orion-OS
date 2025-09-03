#include <stdint.h>
#include <stddef.h>
#include "drivers/vga.h"
#include "core/log.h"
#include "drivers/serial.h"
#include "lib/include/libc.h"
#include "core/io.h"

extern char __git_shortsha[]; /* Optional: linker-provided string */

void kmain(void) {
    // Write directly to VGA to show we're in kmain
    *((volatile uint32_t*)0xb8010) = 0x2f4d2f4b; // "KM" in white on green (Kernel Main)
    
    serial_init();
    
    // Write directly to VGA after serial init
    *((volatile uint32_t*)0xb8014) = 0x2f492f53; // "SI" in white on green (Serial Init)
    
    serial_putc('X');
    
    // Write to VGA after putc
    *((volatile uint32_t*)0xb8018) = 0x2f582f58; // "XX" in white on green (after putc)
    
    for (;;) __asm__ volatile ("hlt");
}
