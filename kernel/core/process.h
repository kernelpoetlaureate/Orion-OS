#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef struct {
    const char *name;
    int pid; // Process ID
    int cpuid; // CPU core ID
    void (*entry_point)(void);
    uint64_t stack_pointer; // Top of the stack
} Process;

Process fork(Process *parent);

#endif // PROCESS_H
