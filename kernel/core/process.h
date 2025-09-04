#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h> // Include for uint64_t

// Define a Process struct
typedef struct {
    const char *name;
    int pid;
    int cpuid; // ID of the CPU core running the process
    void (*entry_point)(void);
    uint64_t stack_pointer; // Pointer to the top of the process's stack
} Process;

// Fork function declaration
Process fork(Process *parent);

// Function to get the current CPU ID
int get_current_cpuid();

#endif // PROCESS_H
