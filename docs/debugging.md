# Debugging & Observability

This document lists the basic debugging workflows for Orion OS: serial capture, GDB attach, and panic handling.

## Serial console (COM1)
Orion initializes COM1 (port 0x3F8) early. You can capture the serial output using `socat` on Linux:

```bash
# Listen on /dev/pts and dump to stdout
socat -d -d pty,raw,echo=0,link=/tmp/orion-serial stdout
# Or connect QEMU's -serial tcp:127.0.0.1:4444,server,nowait and then:
nc 127.0.0.1 4444
```

Example QEMU invocation to expose serial via TCP:

```bash
qemu-system-x86_64 -cdrom iso/orion.iso -serial tcp:127.0.0.1:4444,server,nowait
```

## Panic and logs
- `panic(const char *fmt, ...)` prints the formatted message to the serial console and halts the CPU.
- `LOG(level, fmt, ...)` writes formatted logs to the serial console when `level >= LOG_LEVEL_MIN`.

## GDB flow (local)
1. Build kernel ELF with debug symbols (e.g., `-g` and not stripped).
2. Start QEMU paused and listening for GDB:

```bash
qemu-system-x86_64 -s -S -kernel build/orion.elf
```

3. In another shell run GDB against your ELF:

```bash
gdb build/orion.elf
source .gdbinit
target remote :1234
```

4. If the kernel is loaded at a non-zero link-time address, use the `.gdbinit` helper:

```
load-symbols build/orion.elf 0xffffffff80000000
```

## Symbolized stack trace (stretch)
Planned: a small frame-walking helper walking RBP and printing saved return addresses, then resolving with symbol maps.


## Notes
- Serial is intentionally initialized lazily by `kprintf` and `panic` to minimize early bootstrap dependencies.
- If you use a different serial port or platform, update `kernel/drivers/serial.c` accordingly.
