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

    // Print PMM performance metrics after kernel initialization
    print_pmm_metrics();

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

    // More robust PMM benchmark
    printf("Starting PMM benchmark...\n");
    
    // Reset performance counters
    pmm_cycles_alloc = 0;
    pmm_calls_alloc = 0;
    pmm_cycles_free = 0;
    pmm_calls_free = 0;
    
    // Number of allocations to test
    #define NUM_ALLOCS 100
    void *pages[NUM_ALLOCS];
    
    // Allocate pages
    printf("Allocating %d pages...\n", NUM_ALLOCS);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        pages[i] = alloc_page();
        if (i % 25 == 0) {
            printf("Allocated %d pages so far\n", i);
        }
    }
    
    // Print allocation metrics
    printf("Allocation complete. Metrics:\n");
    printf("PMM debug: alloc_calls=%u, alloc_cycles=%u\n", 
           (unsigned)pmm_calls_alloc, (unsigned)pmm_cycles_alloc);
    if (pmm_calls_alloc > 0) {
        printf("PMM alloc: %u cycles/op\n", 
               (unsigned)(pmm_cycles_alloc / pmm_calls_alloc));
    }
    
    // Reset free counters
    pmm_cycles_free = 0;
    pmm_calls_free = 0;
    
    // Free pages
    printf("Freeing %d pages...\n", NUM_ALLOCS);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        free_page(pages[i]);
        if (i % 25 == 0) {
            printf("Freed %d pages so far\n", i);
        }
    }
    
    // Print free metrics
    printf("Free complete. Metrics:\n");
    printf("PMM debug: free_calls=%u, free_cycles=%u\n", 
           (unsigned)pmm_calls_free, (unsigned)pmm_cycles_free);
    if (pmm_calls_free > 0) {
        printf("PMM free: %u cycles/op\n", 
               (unsigned)(pmm_cycles_free / pmm_calls_free));
    }
    
    printf("Benchmark complete.\n");

    // Print final PMM performance metrics
    print_pmm_metrics();

    // Start the parent process
    parent_process.entry_point();
}
