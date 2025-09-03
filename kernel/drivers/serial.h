#ifndef ORION_SERIAL_H
#define ORION_SERIAL_H

#include <stddef.h>

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);

#endif /* ORION_SERIAL_H */
