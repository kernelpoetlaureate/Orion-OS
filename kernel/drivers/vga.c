#include "vga.h"
#include <stdint.h>


static volatile uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;


#define VGA_WIDTH 80
#define VGA_HEIGHT 25


static size_t vga_row = 0;
static size_t vga_col = 0;

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static const uint8_t DEFAULT_ATTR = 0x07;

void vga_init(void) {
    size_t total_cells = VGA_WIDTH * VGA_HEIGHT; // Define total_cells locally
    uint16_t blank_entry = make_entry(' ', DEFAULT_ATTR); // Precompute the blank cell with a space
    for (size_t i = 0; i < total_cells; ++i) {
        VGA_BUFFER[i] = blank_entry;
    }
    vga_row = 0; vga_col = 0;
}


void vga_putc(char c) {
    if (c == '\n') {
        vga_col = 0; ++vga_row;
        if (vga_row >= VGA_HEIGHT) vga_row = 0;
        return;
    }
    size_t index = vga_row * VGA_WIDTH + vga_col; // Compute linear index once
    VGA_BUFFER[index] = make_entry(c, DEFAULT_ATTR);
    if (++vga_col >= VGA_WIDTH) {
        vga_col = 0; ++vga_row;
        if (vga_row >= VGA_HEIGHT) vga_row = 0;
    }
}

void vga_write(const char *s) {
    while (*s) vga_putc(*s++);
}
