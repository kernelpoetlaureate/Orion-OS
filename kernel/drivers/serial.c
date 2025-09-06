#include "../core/io.h"
#include "serial.h"

#define COM1_PORT 0x3F8

void serial_init(void) {
    // Disable interrupts
    outb(COM1_PORT + 1, 0x00);
    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 3, 0x80);
    // Set divisor to 3 (38400 baud)
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 3, 0x03);
    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 2, 0xC7);
    // IRQs enabled, RTS/DSR set
    outb(COM1_PORT + 4, 0x0B);
}

static inline int serial_transmit_empty() {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_putc(char c) {
    while (!serial_transmit_empty()) { }
    outb(COM1_PORT, (uint8_t)c);
}

void serial_write(const char *s) {
    // Simple, robust serial string writer: converts \n to \r\n and writes bytes
    while (*s) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s);
        s++;
    }
}
