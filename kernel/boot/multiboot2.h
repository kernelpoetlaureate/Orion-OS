#pragma once

#include <stdint.h>
#include <stddef.h>
#include "core/pmm.h"

/* Parse a Multiboot2 info structure at `mbi` and produce an array of
 * canonical usable regions (phys_mem_region_t) in `out` up to max_out.
 * Returns number of usable regions written, or 0 on failure. The parser
 * also accepts Multiboot1 legacy mem_lower/mem_upper if the Multiboot2
 * tag stream is not present. */
size_t parse_multiboot2(void *mbi, phys_mem_region_t *out, size_t max_out);
