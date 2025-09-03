#include "vga.h"
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
static uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;
static size_t vga_row = 0;
static size_t vga_col = 0;

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_init(void) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_BUFFER[y * VGA_WIDTH + x] = make_entry(' ', 0x07);
        }
    }
    vga_row = 0; vga_col = 0;
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_col = 0; ++vga_row;
        if (vga_row >= VGA_HEIGHT) vga_row = 0;
        return;
    }
    VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = make_entry(c, 0x07);
    if (++vga_col >= VGA_WIDTH) { vga_col = 0; ++vga_row; if (vga_row >= VGA_HEIGHT) vga_row = 0; }
}

void vga_write(const char *s) {
    while (*s) vga_putc(*s++);
}
