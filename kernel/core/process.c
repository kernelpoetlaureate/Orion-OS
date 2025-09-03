#include "core/process.h"
#include "lib/include/libc.h"

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
    Process child = {
        .name = "Child Process",
        .pid = next_pid++,
        .cpuid = get_current_cpuid(),
        .entry_point = parent->entry_point
    };
    return child;
}
