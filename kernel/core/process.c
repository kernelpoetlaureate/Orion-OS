#include "core/process.h"
#include "core/pmm.h"
#include <stdint.h>
#include <stdatomic.h>

// Global variable to track the next PID
static atomic_int next_pid = 2;

// Fork function to create a child process
Process fork(Process *parent) {
    void* p = pmm_alloc();
    return (Process){
        .pid = p ? atomic_fetch_add(&next_pid, 1) : -1,
        .cpuid = p ? parent->cpuid : 0,
        .entry_point = p ? parent->entry_point : 0,
        .stack_pointer = p ? (uint64_t)p + PAGE_SIZE : 0
    };
}
