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

extern char __git_shortsha[]; /* Optional: linker-provided string */

// Multiboot information structure
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    // Other fields can be added as needed
} multiboot_info_t;

// Parent process entry point
void parent_process_entry(void) {
    printf("Welcome to Orion OS\n");
    for (;;) __asm__ volatile ("hlt");
}

void kmain(multiboot_info_t *mb_info) {
    LOG_INFO("00000000000000000000000000000000000000000");
    LOG_INFO("kmain: Starting kernel main function");

    // Parse Multiboot information structure
    if (mb_info->flags & 0x1) {
        printf("00000000000000000000000000000000000000000");

        printf("Lower memory: %u KB\n", mb_info->mem_lower);
        printf("Upper memory: %u KB\n", mb_info->mem_upper);
    } else {
        printf("Memory information not available.\n");
    }

    LOG_INFO("kmain: Initializing serial output");
    serial_init();

    // Prompt for PMM type selection
    printf("Select PMM Type:\n");
    printf("1. Fine-grained bitmap (1 bit per page)\n");
    printf("2. Coarse-grained bitmap (1 bit per 16 pages)\n");
    printf("Enter choice (1 or 2): ");
    
    // In a real system, this would read input from keyboard
    // For now, let's use a default value (can be changed manually here)
    int choice = 2; // Default to coarse-grained
    printf("%d\n", choice);
    
    // Set PMM type based on choice
    pmm_type_t pmm_type = (choice == 1) ? PMM_BITMAP_FINE : PMM_BITMAP_COARSE;
    printf("Using %s\n", pmm_get_type_name(pmm_type));

    /*
     * Build a phys_mem_region_t list from Multiboot (legacy) info.
     * The simple multiboot_info_t used here is a legacy struct; for a
     * production Multiboot2 parser you would parse tags. We will attempt
     * to build a minimal map from mem_lower/mem_upper as a fallback.
     */
    phys_mem_region_t map[32];
    size_t map_entries = parse_multiboot2(mb_info, map, 32);
    if (map_entries == 0) {
        LOG_WARN("kmain: multiboot2 parse produced no usable regions, falling back to pmm_init()");
        pmm_init(pmm_type);
    } else {
        LOG_INFO("kmain: parsed %u usable memory regions from Multiboot2", (unsigned)map_entries);
        pmm_init_from_map(map, map_entries, pmm_type);
    }
    pmm_self_test();

    // Print PMM performance metrics after kernel initialization
    print_pmm_metrics();

    // Print the physical memory map managed by the PMM
    printf("Physical Memory Map:\n");
    pmm_print_memory_map();

    LOG_INFO("kmain: Creating parent process");
    // Create the parent process
    Process parent_process = {
        .name = "Parent Process",
        .pid = 1,
        .cpuid = 0, // Set to 0 as a placeholder
        .entry_point = parent_process_entry,
        .stack_pointer = 0  // Parent uses kernel stack
    };

    LOG_INFO("kmain: Forking child process");
    // Fork a child process
    Process child_process = fork(&parent_process);

    // Print PIDs and CPU IDs of both processes
    printf("Parent PID: %d, CPUID: %d\n", parent_process.pid, parent_process.cpuid);
    printf("Child PID: %d, CPUID: %d\n", child_process.pid, child_process.cpuid);

    // Start the parent process
    parent_process.entry_point();
}
