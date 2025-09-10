# Plan for Using Multiboot/Multiboot2 Memory Map from GRUB

Use the Multiboot/Multiboot2 memory map from GRUB to build a canonical list of usable physical ranges, subtract all reserved artifacts (kernel, modules, boot info, ACPI/NVS, framebuffer), then page-align, merge, and feed that into the PMM to place and initialize the bitmap; the plan below walks through data structures, parsing, reservations, and layout choices step by step with GRUB-specific details. Treat only E820/Multiboot type "+Available" as allocatable up front; keep ACPI reclaimable for later, and never touch NVS/reserved/badram, mirroring common OS practice and GRUB’s tag semantics.[1][2][3][4]

## Inputs from GRUB
- Multiboot2 boot information is a sequence of 8‑byte aligned tags providing the memory map, modules, ELF sections, EFI/ACPI info, and framebuffer; verify the magic and length, then iterate tags until the END tag.[3][1]
- The memory map tag carries 64‑bit base/length/type entries derived from E820; type 1 = Available RAM, 3 = ACPI reclaimable, 4 = ACPI NVS, 5 = badram, others reserved; Multiboot1 exposes similar mmap_* fields when present.[5][4]

## Data structures
- Define a simple region struct: { start, end, type }, where type ∈ {USABLE, RESERVED, ACPI_RECLAIM, ACPI_NVS, BADRAM, MODULE, KERNEL, MBI, FRAMEBUFFER} for canonicalization and easy subtraction/merging.[6][4]
- Maintain vectors: all_regions (raw from tags and later reservations), usable_regions (post-canonicalization), and reserved_regions (for diagnostics and to drive PMM marking) to cleanly separate parsing from allocation.[7][3]

## Parsing sequence
- Early boot: capture pointer to Multiboot2 info struct and iterate tags with the spec’s size/8‑byte alignment rule; collect memory map entries, module tags (base/length), ELF sections for the kernel image extents, and framebuffer tag if graphics is enabled.[1][3]
- For Multiboot1 fallback, use flags to confirm mmap validity (bit 6) and iterate fixed-size entries at mmap_addr..mmap_addr+mmap_length, as documented by the spec and OSDev references.[8][5]

## Region classification
- Convert each memory map entry to a region with normalized end = base + length; classify type 1 as USABLE, type 3 as ACPI_RECLAIM, type 4 as ACPI_NVS, type 5 as BADRAM, else RESERVED to avoid accidental use of firmware or MMIO ranges.[2][4]
- Record module ranges and the entire boot info (MBI) region as RESERVED, since the spec notes the kernel must not overwrite these until they are explicitly released by the kernel after consumption.[6][3]

## Kernel and artifacts reservation
- Derive the kernel’s physical footprint from the ELF sections tag (or linker symbols) and add a KERNEL region to reserved_regions; do the same for the MBI struct and each module tag to avoid clobbering them during PMM init.[9][3]
- If a framebuffer tag is present, reserve the framebuffer’s physical range; this prevents mapping it as general RAM and matches GRUB’s graphics tag semantics.[3][7]

## Canonicalization pipeline
- Normalize: sort regions by start, merge adjacent/overlapping regions of the same type, and clip zero-length; keep types intact to preserve ACPI vs RESERVED semantics for later phases.[10][4]
- Subtract reserved from usable: iteratively subtract KERNEL, MBI, MODULE, FRAMEBUFFER, ACPI_NVS, BADRAM, and non-1 types from USABLE regions, splitting as needed to produce a clean set of page-granular usable ranges.[4][10]

## Page alignment
- Round each usable region up/down to page boundaries: start_aligned = ceil(start, PAGE_SIZE), end_aligned = floor(end, PAGE_SIZE); discard regions smaller than one page post-alignment to keep PMM logic simple and correct.[7][4]
- Keep exact (non-rounded) boundaries only for diagnostics; the PMM should operate on aligned PFNs to avoid partial-page aliasing issues early in boot.[4][1]

## Bitmap placement strategy
- Compute total usable pages and the bitmap size; place the bitmap in the first sufficiently large usable region after the kernel image, then subtract its range from that region and mark it RESERVED to avoid self-overwrite by the PMM.[3][7]
- Prefer to co-locate the bitmap near the kernel to reduce TLB/cache misses and keep the PMM’s metadata footprint contiguous; this mirrors common OS-dev guidance for early allocators.[8][7]

## PMM initialization from the map
- Initialize all bits to “used,” then clear bits only for aligned usable PFNs; after that, set bits again for all reserved artifacts (kernel, modules, MBI, framebuffer, bitmap) to ensure one source of truth (the bitmap) reflects the final state.[11][7]
- Sum total_pages/used_pages from the canonicalized PFN sets rather than arithmetic over [MEMORY_START, MEMORY_END), which eliminates off-by-one and tail-fragment errors on non-multiple-of-page ranges.[11][4]

## ACPI reclaimable handling
- Treat ACPI reclaimable (type 3) as RESERVED until ACPI tables are parsed and copied; later, add those ranges back as USABLE by calling a reserve_release() routine that flips their bits to free in the PMM.[2][4]
- Never allocate from ACPI NVS (type 4) or BADRAM (type 5); they must remain out of the general allocator for correctness and reliability.[2][4]

## Edge cases and robustness
- Handle overlapping/duplicate E820 entries by sorting and coalescing with defined precedence: RESERVED > ACPI_NVS > BADRAM > ACPI_RECLAIM > USABLE to avoid leaks of reserved memory into usable pools.[10][4]
- Accept systems without Multiboot memory map by failing fast or falling back to a minimal static map only in development builds; the Multiboot spec and OSDev docs strongly recommend using the provided map.[7][3]

## Testing and diagnostics
- Print the raw and canonicalized region lists at boot (type, start..end, pages) and the final totals; include the MBI, kernel, modules, and bitmap reservations explicitly for validation on real hardware and QEMU.[12][7]
- Add assertions for alignment and non-overlap after canonicalization; compare against the Multiboot2 tag dump to catch bugs in subtraction/merging logic early.[8][3]

## Optional UEFI considerations
- GRUB may provide an EFI memory map tag in lieu of a legacy E820 map; if present, parse that tag and translate EFI types to the same USABLE/RESERVED/ACPI classes before canonicalization to keep the PMM path unified.[1][3]
- The ACPI spec clarifies AddressRangeACPI (reclaimable after parsing) versus AddressRangeNVS (never general-allocatable), which should drive your type mapping from EFI to PMM classes.[2]

## Minimal implementation checklist
- Verify Multiboot2 magic; iterate 8‑byte aligned tags; collect MemoryMap, Modules, ELF sections, Framebuffer; record MBI base/size.[1][3]
- Build raw regions; classify types (1=USABLE, 3=ACPI_RECLAIM, 4=NVS, 5=BADRAM, else RESERVED); add KERNEL/MODULE/MBI/FRAMEBUFFER as RESERVED.[4][7]
- Sort, merge, subtract reserved from usable, page-align, drop sub-page tails; compute total usable PFNs.[10][4]
- Place bitmap in a usable region, reserve it, then initialize the PMM bitmap from the final PFN sets and expose reserve_range/release_range utilities for late reservations (e.g., ACPI reclaim release).[11][7]

This plan aligns with Multiboot2’s tag model, E820 type semantics, and how mature kernels build a canonical physical map before enabling general allocation, ensuring the PMM never touches reserved firmware/MMIO space and that all allocator metadata is safely placed and tracked from the outset.[3][2]

[1](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
[2](https://uefi.org/specs/ACPI/6.5/15_System_Address_Map_Interfaces.html)
[3](https://www.gnu.org/software/grub/manual/multiboot2/html_node/Boot-information-format.html)
[4](https://wiki.osdev.org/Detecting_Memory_(x86))
[5](https://stackoverflow.com/questions/71964716/what-do-the-mmaps-provided-in-multiboot-information-mean)
[6](https://docs.rs/multiboot2/latest/multiboot2/struct.MemoryMapTag.html)
[7](https://wiki.osdev.org/Multiboot)
[8](https://thuc.space/posts/os_kernel_assembly/)
[9](https://www-old.cs.utah.edu/flux/oskit/OLD/html/doc-0.96/node426.html)
[10](https://research.cs.wisc.edu/adsl/Software/tratr/.scripts/tratr/arch/x86/kernel/e820.c)
[11](https://wiki.osdev.org/Page_Frame_Allocation)
[12](https://forum.osdev.org/viewtopic.php?t=32296)
[13](http://arxiv.org/pdf/2307.14471.pdf)
[14](https://arxiv.org/pdf/2409.10946.pdf)
[15](https://pmc.ncbi.nlm.nih.gov/articles/PMC4162891/)
[16](http://ispras.ru/proceedings/docs/2018/30/3/isp_30_2018_3_121.pdf)
[17](https://static-content.springer.com/esm/art:10.1186/2193-1801-3-494/MediaObjects/40064_2014_1199_MOESM1_ESM.pdf)
[18](https://pmc.ncbi.nlm.nih.gov/articles/PMC3565456/)
[19](https://dl.acm.org/doi/pdf/10.1145/3565026)
[20](http://arxiv.org/pdf/2403.04539.pdf)
[21](https://arxiv.org/html/2409.11220v1)
[22](https://arxiv.org/pdf/2305.07440.pdf)
[23](http://arxiv.org/pdf/2407.20628.pdf)
[24](https://arxiv.org/pdf/2502.02437.pdf)
[25](http://arxiv.org/pdf/2503.17602.pdf)
[26](http://arxiv.org/pdf/2104.07699.pdf)
[27](http://arxiv.org/pdf/2409.13327.pdf)
[28](https://pmc.ncbi.nlm.nih.gov/articles/PMC10656460/)
[29](https://dl.acm.org/doi/pdf/10.1145/3613424.3614296)
[30](https://arxiv.org/pdf/1212.1787.pdf)
[31](https://docs.rs/multiboot2/latest/multiboot2/enum.MemoryAreaType.html)
[32](https://www.youtube.com/watch?v=fiSsROFBZ6s)
[33](https://docs.rs/multiboot2)
[34](https://asterinas.github.io/api-docs/0.4.2/multiboot2/index.html)
[35](https://stackoverflow.com/questions/68475415/multiboot2-header-comes-too-late-in-elf-file-to-large-offset-even-if-it-is)
[36](https://www.reddit.com/r/osdev/comments/uwao71/how_do_i_get_memory_size_using_grub/)
[37](https://forum.osdev.org/viewtopic.php?t=29851)
[38](https://f.osdev.org/viewtopic.php?t=32530)
[39](https://unikraft.org/blog/2024-06-17-gsoc-multiboot2)
