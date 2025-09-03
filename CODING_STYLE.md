# Coding Style for Orion OS

This document captures the baseline coding conventions used in Orion OS to keep the codebase consistent and reviewable.

## C style
- Indent with 4 spaces.
- Max line length: 100 characters.
- Braces: K&R style for function definitions and control blocks.
- Use `/* */` for multi-line comments and `//` for short single-line comments.

## Headers
- Use include guards in headers: `ORION_<PATH>_H` style.
- Public kernel headers go under `kernel/include/` or the subsystem's `include/` folder.

## Naming
- Arch-specific symbols: `arch_x86_<name>`.
- Module init function: `int <module>_init(void);` (returns 0 on success).
- Panic function: `void panic(const char *fmt, ...);` — should be reachable from all code.

## Files & Organization
- One top-level concept per file where sensible.
- Keep public APIs minimal and documented in a header.

## Build & Tests
- Host-side tests are allowed in `tests/` and should be buildable on a normal Linux dev host.
- Kernel-only code must avoid depending on the host libc.

## Review checklist
- Does the code include/update a design note for architecture changes?
- Are public symbols documented in headers?
- Do tests cover important edge cases for pure logic?
