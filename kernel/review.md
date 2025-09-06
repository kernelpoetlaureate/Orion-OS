I'll search the codebase for Limine request structs and existing bootloader integration to answer precisely (what protocol, and what request variables exist). I'll retrieve any matching files and snippets. Now read limine.h around memmap and hhdm structs to show exact code. I'll gather and report the exact boot/bootloader/memory-access ingredients found in the repo so you can proceed with a precise PMM plan.

Checklist
- Bootloader and protocol — Reported
- Memory map access code (limine request + usage) — Provided
- HHDM availability and request code — Provided
- Kernel location method and request code — Provided
- Core utilities: printf, memset, panic, serial — Reported

What I inspected (progress): I looked at limine.h, _start.asm, panic.c, mem.c, and libc.h to confirm available interfaces and implementations.

Answers

1) Bootloader
- Which: The repository includes and uses Limine (see limine and limine.cfg). There is also a small Multiboot2 header in _start.asm (likely for GRUB compatibility), but the available boot-info request structs and build artifacts indicate Limine is the intended primary bootloader.
- Protocol: Use the Limine protocol (Limine requests / Limine ABI). Use the Limine request structs defined in limine.h.

2) Memory map access (C code to read memmap)
- Request declaration to place in your kernel source (exactly as used with Limine):
```
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};
```
- Usage pattern:
  - Check `if (memmap_request.response == NULL) panic("memmap missing");`
  - Read `memmap_request.response->entry_count`
  - Access entries via `memmap_request.response->entries[i]` (each is `struct limine_memmap_entry *`)
  - Each entry has fields: `entry->base`, `entry->length`, `entry->type` (type constants like `LIMINE_MEMMAP_USABLE` are in limine.h).

3) HHDM (Higher-Half Direct Map)
- Does it exist? Yes — limine.h contains `LIMINE_HHDM_REQUEST` and `struct limine_hhdm_response` (has `.offset`).
- Request declaration:
```
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};
```
- Usage:
  - Check `if (hhdm_request.response == NULL) { /* fall back / panic */ }`
  - The HHDM offset is `hhdm_request.response->offset` — convert physical to virtual with `void *phys_to_virt(uint64_t p) { return (void *)(p + hhdm_offset); }` where `hhdm_offset` is the `offset` above.

4) Kernel physical location & size
- Limine provides `LIMINE_KERNEL_FILE_REQUEST`. You can declare:
```
static volatile struct limine_kernel_file_request kernel_file_request = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0
};
```
- Usage:
  - `kernel_file_request.response->kernel_file` is a `struct limine_file *`
  - The kernel's physical load address is `kernel_file_request.response->kernel_file->address`
  - The kernel size is `kernel_file_request.response->kernel_file->size`
- Alternative: memmap entries sometimes mark kernel pages with `LIMINE_MEMMAP_KERNEL_AND_MODULES` — you can also scan memmap entries and treat that range as kernel-reserved.

5) Core utilities available in this tree
- Logging/printing:
  - `printf` is available and used in kmain.c and tests. Declaration in libc.h and implementation linked via printf.c.
- Basic libc:
  - `memset`, `memcpy`, `memcmp`, `strcmp` exist (see mem.c).
- Panic:
  - `panic(const char *fmt, ...)` exists at panic.c. It initializes serial, writes a message, then halts with `hlt` loop.
- Serial:
  - `serial_init()`, `serial_write()` exist (see serial.c).

Next (what I can do for you)
- If you confirm, I will produce the Phase 1–4 PMM recipe tailored to this exact environment (Limine HHDM available, kernel_file request available, `printf`/`memset`/`panic` present). I can also generate the starter `kernel/core/pmm.h` + `kernel/core/pmm.c` skeleton and a small self-check in kmain.c to verify memmap + HHDM reading on boot.

Which next action do you want: (A) full multi-phase PMM plan + code skeleton, or (B) immediate implementation of the PMM init skeleton and a small runtime smoke test?