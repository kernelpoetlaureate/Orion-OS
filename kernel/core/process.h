#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

// Define a Process struct
typedef struct {
    const char *name;
    int pid;
    int cpuid; // ID of the CPU core running the process
    void (*entry_point)(void);
} Process;

// Fork function declaration
Process fork(Process *parent);

// Function to get the current CPU ID
int get_current_cpuid();

#endif // PROCESS_H
