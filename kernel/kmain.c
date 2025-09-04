#include <stdint.h>
#include <stddef.h>
#include "drivers/vga.h"
#include "core/log.h"
#include "drivers/serial.h"
#include "lib/include/libc.h"
#include "core/io.h"
#include "core/process.h"
#include "core/pmm.h"

extern char __git_shortsha[]; /* Optional: linker-provided string */

// Parent process entry point
void parent_process_entry(void) {
    printf("Welcome to Orion OS\n");
    for (;;) __asm__ volatile ("hlt");
}

void kmain(void) {
    LOG_INFO("kmain: Starting kernel main function");

    // Fetch the current CPU ID for the parent process
    int parent_cpuid = get_current_cpuid();

    LOG_INFO("kmain: Initializing serial output");
    serial_init();

    // Initialize PMM first before creating processes
    pmm_init();
    pmm_run_tests();

    LOG_INFO("kmain: Creating parent process");
    // Create the parent process
    Process parent_process = {
        .name = "Parent Process",
        .pid = 1,
        .cpuid = parent_cpuid,
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
