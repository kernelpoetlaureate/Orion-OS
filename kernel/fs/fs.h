#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <stdint.h>
#include <stddef.h>

#define FS_NAME_MAX 256


struct block {
    size_t size;       // Size of the block
    void *address;     // Address of the block
    int process_id;    // ID of the process the block belongs to
};

int fs_init(void);

#endif