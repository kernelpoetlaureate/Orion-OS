#ifndef ORION_VGA_H
#define ORION_VGA_H

#include <stddef.h>

void vga_init(void);
void vga_putc(char c);
void vga_write(const char *s);

#endif /* ORION_VGA_H */
