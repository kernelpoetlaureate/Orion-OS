#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <stdint.h>
#include <stddef.h>

#define FS_NAME_MAX 256
#define MAX_BLOCKS 1024
#define RAMDISK_BLOCK_SIZE 4096


struct block {
    size_t size;       // Size of the block
    void *address;     // Address of the block
    int process_id;    // ID of the process the block belongs to
};

int fs_init(void);
int read_blocks(int dev, size_t lba, size_t count, void *buf);
int write_blocks(int dev, size_t lba, size_t count, const void *buf);
int fs_write_string(size_t lba, const char *s);
int fs_read_string(size_t lba, char *dst, size_t dst_len);

#endif