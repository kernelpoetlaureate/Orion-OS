#include "fs/fs.h"
#include "drivers/serial.h" // Corrected path for serial_write
#include "drivers/vga.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define MAX_BLOCKS 1024
#define RAMDISK_BLOCK_SIZE 4096

// Array representing all the blocks in the system (on HDD)
struct block blocks[MAX_BLOCKS];
static uint8_t ramdisk[MAX_BLOCKS * RAMDISK_BLOCK_SIZE];

void init_blocks() {
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
        blocks[i].size = 0;
        blocks[i].address = NULL;
        blocks[i].process_id = -1; // -1 indicates the block is free
    }
}

// Initrd (initial RAM disk) support
static const uint8_t *initrd_base = NULL;
static size_t initrd_size = 0;

// Call this at boot with the initrd address and size
void init_ramdisk(const void *base, size_t size) {
    initrd_base = (const uint8_t *)base;
    initrd_size = size;
}

// Tiny block device API
// dev: device identifier (opaque for now)
// lba: logical block address
// count: number of blocks
// buf: buffer to read/write
int read_blocks(int dev, size_t lba, size_t count, void *buf) {
    (void)dev; // Only one device for now
    size_t offset = lba * RAMDISK_BLOCK_SIZE;
    size_t bytes = count * RAMDISK_BLOCK_SIZE;
    if (initrd_base) {
        if (offset + bytes > initrd_size) return -1;
        memcpy(buf, initrd_base + offset, bytes);
        return 0;
    }
    // Fall back to in-memory ramdisk
    if (offset + bytes > sizeof(ramdisk)) return -1;
    memcpy(buf, &ramdisk[offset], bytes);
    return 0;
}

int write_blocks(int dev, size_t lba, size_t count, const void *buf) {
    (void)dev; // Read-only
    size_t offset = lba * RAMDISK_BLOCK_SIZE;
    size_t bytes = count * RAMDISK_BLOCK_SIZE;
    // If initrd is present, treat it as read-only
    if (initrd_base) return -1;
    if (offset + bytes > sizeof(ramdisk)) return -1;
    memcpy(&ramdisk[offset], buf, bytes);
    return 0;
}

// Simple test for RAM-disk block API
int test_ramdisk() {
    // Prepare a test buffer simulating an initrd
    static uint8_t test_blob[RAMDISK_BLOCK_SIZE * 2] = {0};
    for (size_t i = 0; i < sizeof(test_blob); ++i) test_blob[i] = (uint8_t)i;
    init_ramdisk(test_blob, sizeof(test_blob));

    uint8_t read_buf[RAMDISK_BLOCK_SIZE] = {0};
    if (read_blocks(0, 1, 1, read_buf) != 0) return -1; // Read block 1

    // Check that the data matches what we expect
    for (size_t i = 0; i < RAMDISK_BLOCK_SIZE; ++i) {
        if (read_buf[i] != (uint8_t)(i + RAMDISK_BLOCK_SIZE)) return -2;
    }
    return 0; // Success
}

static void vga_puts_local(const char *s) {
    while (*s) {
        vga_putc(*s++);
    }
}

static void vga_write_direct(const char *s) {
    volatile uint16_t *video = (volatile uint16_t *)0xB8000;
    size_t pos = 0;
    while (*s) {
        char c = *s++;
        if (c == '\n') {
            pos = ((pos / 80) + 1) * 80;
            continue;
        }
        video[pos++] = (uint16_t)c | (uint16_t)0x0700; // ascii + attribute
    }
}

// Example FS init function that calls the RAM-disk test
int fs_init(void) {
     /* Initialize block table and in-memory ramdisk for read/write tests.
         We deliberately avoid calling test_ramdisk()/init_ramdisk here because
         that sets `initrd_base` (read-only view) which would prevent writes
         being visible to subsequent reads. For the simple demo (hello world)
         we use the in-memory `ramdisk` buffer as the device backing. */
     init_blocks();
     memset(ramdisk, 0, sizeof(ramdisk));

     serial_write("[fs] initialized in-memory ramdisk\n");
     vga_puts_local("[fs] initialized in-memory ramdisk\n");
     vga_write_direct("[fs] initialized in-memory ramdisk\n");

     return 0;
}

// Convenience: write a null-terminated string to block `lba` (single block)
int fs_write_string(size_t lba, const char *s) {
    char buf[RAMDISK_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    size_t len = strlen(s);
    if (len >= sizeof(buf)) return -1;
    memcpy(buf, s, len);
    return write_blocks(0, lba, 1, buf);
}

// Convenience: read a string from block `lba` into dst (dst_len bytes)
int fs_read_string(size_t lba, char *dst, size_t dst_len) {
    if (dst_len == 0) return -1;
    char buf[RAMDISK_BLOCK_SIZE];
    if (read_blocks(0, lba, 1, buf) != 0) return -1;
    // Ensure null termination
    buf[RAMDISK_BLOCK_SIZE - 1] = '\0';
    size_t copy = strnlen(buf, RAMDISK_BLOCK_SIZE) + 1;
    if (copy > dst_len) copy = dst_len - 1;
    memcpy(dst, buf, copy);
    dst[copy] = '\0';
    return 0;
}

