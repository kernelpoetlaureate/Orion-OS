#include "core/process.h"
#include "core/pmm.h"
#include "core/log.h"
#include "lib/include/libc.h"
#include <stdint.h>

// Global variable to track the next PID
int next_pid = 2;

// Function to get the current CPU ID
int get_current_cpuid() {
    unsigned int eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    printf("Debug: CPUID eax=%u, ebx=%u, ecx=%u, edx=%u\n", eax, ebx, ecx, edx);
    return (ebx >> 24) & 0xFF; // Extract APIC ID
}

// Fork function to create a child process
Process fork(Process *parent) {
    printf("FORK: Starting fork function\n");
    
    printf("FORK: About to create child process struct\n");
    Process child;
    child.name = "Child Process";
    child.pid = next_pid++;
    child.cpuid = parent->cpuid;
    child.entry_point = parent->entry_point;
    child.stack_pointer = 0; // Will be set later
    
    printf("FORK: Child process created with PID %d\n", child.pid);

    // Allocate memory for the child process's stack
    void* stack = pmm_alloc();
    if (stack == NULL) {
        PANIC("Failed to allocate memory for child process stack");
    }

    // Set up the stack pointer for the child process
    child.stack_pointer = (uint64_t)stack + PAGE_SIZE; // Stack grows downward

    // Log the allocation
    LOG_INFO("Allocated stack for child process (PID: %d) at 0x%p", child.pid, stack);

    return child;
}
