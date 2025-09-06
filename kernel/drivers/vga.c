#include "vga.h"
#include <stdint.h>

// Low-Level VGA Text Driver
// This file provides low-level functions to interact with the VGA text-mode framebuffer.
// All high-level printing functions ultimately call these functions to display text on the screen.

// VGA_BUFFER: Points to the 80x25 text-mode framebuffer at memory address 0xB8000.
// Purpose: Directly writes characters and attributes to video memory.
static uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;

// VGA_WIDTH (80): Represents the number of columns (characters per row) in the VGA text mode.
// VGA_HEIGHT (25): Represents the number of rows (lines) in the VGA text mode.
#define VGA_WIDTH 80
#define VGA_HEIGHT 25


static size_t vga_row = 0;
static size_t vga_col = 0;

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

// vga_init: Initializes the VGA buffer by clearing the screen and setting default attributes.
// Purpose: Prepares the screen for output by resetting the cursor and clearing old data.
void vga_init(void) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_BUFFER[y * VGA_WIDTH + x] = make_entry('X', 0x1F); // White text on blue background
        }
    }
    vga_row = 0; vga_col = 0;
}

// vga_putc: Writes a single character and its attribute to the VGA buffer.
// Purpose: Handles character output, including cursor movement and line wrapping.
// This is the core function for low-level text output.
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
