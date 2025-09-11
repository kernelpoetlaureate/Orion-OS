#include <stdint.h>
#include <stddef.h>
#include "drivers/vga.h"
#include "core/log.h"
#include "drivers/serial.h"
#include "lib/include/libc.h"
#include "core/io.h"
#include "core/process.h"
#include "core/pmm.h"
#include "boot/multiboot2.h"
#include "fs/fs.h"
#include "lib/printf.h"

extern char __git_shortsha[];

void parent_process_entry(void) {
    printf("Welcome to Orion OS\n");
    for (;;) __asm__ volatile ("hlt");
}

void kmain(void *mb_info) {
    serial_init();
    vga_init();
    printf("==== Orion OS Kernel Boot ====" "\n");

    phys_mem_region_t map[32];
    size_t map_entries = parse_multiboot2(mb_info, map, 32);

    #define ORION_PMM_POLICY PMM_BITMAP_FINE

    if (map_entries == 0) {
        pmm_init(ORION_PMM_POLICY);
    } else {
        pmm_init_from_map(map, map_entries, ORION_PMM_POLICY);
    }

    Process parent = {
        .entry_point = parent_process_entry
    };

    int fs_status = fs_init();
    if (fs_status == 0) {
        serial_write("[kernel] fs_init success\n");
    } else {
        serial_write("[kernel] fs_init failed\n");
    }

    parent.entry_point();
}
