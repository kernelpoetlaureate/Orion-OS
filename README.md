# Orion OS

> “Orion: A constellation used for navigation. This project aims to be the same for mastering operating system internals.”

Orion OS is a disciplined, monolithic x86_64 kernel built from first principles with a professional engineering approach: reproducible toolchains, explicit design artifacts, test harnesses, structured milestones, and introspective diagnostics.

---

## Table of Contents
- [Mission](#mission)
- [Core Principles](#core-principles)
- [Current Status](#current-status)
- [Roadmap & Milestones](#roadmap--milestones)
- [Architecture Overview](#architecture-overview)
- [Repository Structure](#repository-structure)
- [Build & Run](#build--run)
- [Development Environment](#development-environment)
- [Subsystem Summaries](#subsystem-summaries)
- [Diagnostics & Debugging](#diagnostics--debugging)
- [Design Documents](#design-documents)
- [Educational Focus](#educational-focus)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

---

## Mission
To provide a clearly documented, stepwise implementation of a 64-bit operating system kernel that serves both as:
1. A personal mastery journey into systems internals.
2. A portfolio artifact demonstrating architectural rigor, low-level competence, and communication clarity.

---

## Core Principles
| Principle        | Description |
|------------------|-------------|
| Reproducibility  | Cross-compiler, containerized build, deterministic outputs. |
| Transparency     | Every major feature has a design note explaining tradeoffs. |
| Instrumentation  | Early logging, panic dumps, optional stack tracing. |
| Incrementality   | Each tag (`v0.x`) is self-consistent and documented. |
| Separation       | Arch-specific vs. generic layers cleanly divided. |
| Verification     | Host-side tests for pure logic; integration harness via QEMU. |

---

## Current Status
**Last Release:** `v0.1` (Bootstrap & Hello Kernel)  
**Live Features:**
- Higher-half kernel entry via Limine
- VGA + serial dual-channel logging
- Early panic framework
- Minimal freestanding libc primitives

**In Progress:** Physical memory manager (bitmap allocator)  
**Next Planned Tag:** `v0.2` (PMM + Paging foundations)

---

## Roadmap & Milestones
| Tag  | Focus                                      | Key Deliverables |
|------|--------------------------------------------|------------------|
| v0.1 | Bootstrap + Console                        | Limine boot, linker script, Hello output |
| v0.2 | Memory Foundations                         | PMM bitmap, paging scaffolding, heap stub |
| v0.3 | Interrupts & Exceptions                    | IDT, fault handlers, PIC remap, timer, keyboard |
| v0.4 | Multitasking                               | Task structs, context switch, preemption |
| v0.5 | User Mode & Syscalls                       | Ring 3 transition, syscall table, basic ABI |
| v0.6 | Initrd + VFS Layer                         | RAM FS loader, file read syscall |
| v0.7 | Device & Console Abstraction               | Unified console backend, improved input |
| v0.8 | SMP & Synchronization                      | AP bring-up, spinlocks, per-CPU state |
| v0.9 | Hardening & Metrics                        | Audits, stack guards, improved logging |
| v1.0 | Showcase Release                           | Documentation polish, blog index |

(See `docs/roadmap.md` for deep detail.)

---

## Architecture Overview
**Design Style:** Monolithic kernel with clearly namespaced subsystems; user mode introduced after robust kernel primitives.

**Major Layers (planned):**
- Boot & Entry: Limine + early assembly trampoline
- Memory: PMM (bitmap) → VMM (4-level paging) → Heap
- Interrupt Architecture: IDT, exception stubs, IRQ routing
- Scheduling: Round-robin preemptive, later SMP aware
- Syscalls: Minimal stable surface (write, yield, getpid ... expandable)
- VFS: Generic node ops atop initrd, extensible to block FS
- Devices: Serial, timer, keyboard, (future: framebuffer, disk, NIC)
- SMP: AP startup + locking primitives

A detailed diagram will be added in `docs/architecture-diagram.md`.

---

## Repository Structure
```
kernel/
  arch/x86_64/
    boot/          # Entry asm + bootstrap headers
    mm/            # Arch paging, tables
    interrupts/    # IDT, ISR stubs
  core/            # Generic kernel subsystems
  drivers/         # Device-specific code (serial, timer, keyboard)
  lib/             # Freestanding utilities (string, memory)
docs/
scripts/
tests/
toolchain/
assets/            # Screenshots, diagrams
linker.ld
limine.cfg
Makefile
```

---

## Build & Run

Prerequisites:
- `qemu-system-x86_64`
- `xorriso`, `grub-mkrescue` or Limine ISO tools
- Cross GCC (see `docs/toolchain.md`) OR use container

Quick Start:
```bash
make toolchain   # Optional: build cross-compiler
make             # Build kernel ELF + ISO
make run         # Launch QEMU
make debug       # Launch QEMU paused + wait for GDB
```

Headless + serial capture:
```bash
./scripts/qemu-run.sh --serial-log build/serial.log
```

---

## Development Environment
Containerized (Dev Container / Docker):
```bash
docker build -t orion-dev .
docker run --rm -it -v $(pwd):/workspace orion-dev /bin/bash
```
(See `Dockerfile` + `docs/environment.md`.)

---

## Subsystem Summaries

### Boot & Entry
Loads via Limine, transitions to higher-half, sets stack, calls `kmain()`.  
Design: `docs/design-boot-entry.md`

### Memory
PMM → Bitmap where 1 bit = 4KiB frame. VMM will supply `map/unmap` primitives.  
Design: `docs/design-memory.md`

### Interrupts
IDT entries auto-generated; trap frame passed into uniform C handler.  
Design: `docs/design-interrupts.md`

*(Further sections evolve as implemented.)*

---

## Diagnostics & Debugging
- Serial (COM1) logging enabled early
- Log levels: TRACE/DEBUG/INFO/WARN/ERROR/PANIC (compile-time filter WIP)
- Panic path prints registers + partial stack (future: symbolization)
- GDB script: `.gdbinit` loads symbols and sets convenience aliases
- Integration tests: headless runs grepping for boot banner

Reference: `docs/debugging.md`

---

## Design Documents
| Subsystem  | Status | File |
|------------|--------|------|
| Boot       | Draft  | `docs/design-boot-entry.md` |
| Memory     | Pending| `docs/design-memory.md` |
| Interrupts | Pending| `docs/design-interrupts.md` |
| Scheduler  | Planned| `docs/design-scheduler.md` |

A PR adding a subsystem MUST include or update a design doc.

---

## Educational Focus
This project favors explicitness over cleverness. Each release is meant to isolate *conceptual digestion*:
- v0.2: Page frames + address translation mental model
- v0.3: CPU exception semantics & vector discipline
- v0.4: Context switch invariants & register discipline

---

## Contributing
Issue templates and contribution guidelines will evolve. For now:
1. Draft or update a design doc before major code.
2. Keep PRs focused (one subsystem or refactor).
3. Provide before/after rationale in the description.

See: `CONTRIBUTING.md` (to be added).

---

## License
(Choose one—MIT, BSD-2-Clause, BSD-3-Clause are common for kernels. TODO.)

---

## Acknowledgements
- OSDev.org Wiki (invaluable reference)
- Limine project maintainers
- Authors of Intel/AMD architecture manuals
- Open-source hobby kernels studied for inspiration (not copied)

---

## Status Badge Ideas (TBD)
- Build (CI) | Lines of Code | Docs Coverage | Last Tag | Architecture Diagram

---

> “Navigation requires fixed points. Orion OS aims to make the fundamentals of kernel construction those fixed points.”
