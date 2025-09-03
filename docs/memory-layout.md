# Memory Layout & Higher-Half Plan

This document explains the kernel's memory layout and the rationale for a higher-half kernel mapping.

## Rationale
Historically, kernels live in the lower physical addresses, but a higher-half kernel (mapped to a high virtual address such as 0xffffffff80000000) provides a consistent virtual address space independent of where the kernel is loaded physically. This simplifies address-space separation and simplifies the design of user/kernel transitions.

## Current Linker Layout
- The kernel is linked with a load address of `0x100000` (1 MiB) in the minimal `linker.ld`. This is a physical load address for a simple early bootstrap.
- At a later stage, a proper virtual mapping will be established and the kernel will be relocated or run with identity mapping for the early phase.

## Plan
1. Early boot: run with identity mapped pages while switching to long mode and setting up paging.
2. Create a simple page table that maps kernel virtual base (e.g., `0xffffffff80000000`) to the physical load address.
3. Once page tables are active, switch to higher-half virtual addresses and use the virtual base for all kernel symbols.

## Developer notes
- The linker script will evolve to reflect the virtual link-time address once higher-half mapping is enabled.
- Always produce a map file during linking for review (`ld -M -T linker.ld ...`).
