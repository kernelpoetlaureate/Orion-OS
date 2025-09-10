// kernel/core/pmm.c

/* Simple, short PMM implementation.
 * - Small bitmap allocator (fine or coarse).
 * - No profiling, minimal logging.
 *
 * Updated to support parsing firmware/bootloader memory maps (e820/UEFI).
 * Use pmm_init_from_map() to initialize with a memory map; pmm_init()
 * remains as a backwards-compatible wrapper (emulates a single usable region
 * and logs a warning).
 */

#include "core/pmm.h"
#include "core/log.h"
#include "lib/include/libc.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MIN_MEMORY_START 0x100000ULL
#define DEFAULT_MEMORY_END   0x40000000ULL
#define KERNEL_START 0x100000ULL
#define KERNEL_SIZE  0x200000ULL
#define BLOCK_SIZE 32

static uint8_t *bitmap = NULL;
static size_t bitmap_size_bytes = 0;
static size_t total_pages = 0;
static size_t used_pages = 0;
static pmm_type_t current_pmm_type = PMM_BITMAP_FINE;

/* physical memory range handled by PMM (set at init from map) */
static uint64_t phys_memory_start = MIN_MEMORY_START;
static uint64_t phys_memory_end = DEFAULT_MEMORY_END;

/* Performance variables declared in header - keep zeroed for compatibility */
uint64_t pmm_cycles_alloc = 0;
uint64_t pmm_calls_alloc = 0;
uint64_t pmm_cycles_free = 0;
uint64_t pmm_calls_free = 0;

// phys_mem_region_t is declared in pmm.h

/* tiny bitmap helpers */
static inline void bit_set(size_t i) { bitmap[i >> 3] |= (1u << (i & 7)); }
static inline void bit_clear(size_t i) { bitmap[i >> 3] &= ~(1u << (i & 7)); }
static inline int bit_test(size_t i) { return (bitmap[i >> 3] >> (i & 7)) & 1; }

pmm_type_t pmm_get_type(void) { return current_pmm_type; }

const char* pmm_get_type_name(pmm_type_t t) {
    return t == PMM_BITMAP_COARSE ? "coarse" : "fine";
}

/* internal helpers for marking ranges */
static void mark_range_used_fine(uint64_t start, uint64_t end) {
    if (end <= phys_memory_start || start >= phys_memory_end) return;
    if (start < phys_memory_start) start = phys_memory_start;
    if (end > phys_memory_end) end = phys_memory_end;
    size_t p_start = (start - phys_memory_start) / PAGE_SIZE;
    size_t p_end = (end - phys_memory_start + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t p = p_start; p < p_end && p < total_pages; p++) {
        if (!bit_test(p)) { bit_set(p); used_pages++; }
    }
}

static void mark_range_free_fine(uint64_t start, uint64_t end) {
    if (end <= phys_memory_start || start >= phys_memory_end) return;
    if (start < phys_memory_start) start = phys_memory_start;
    if (end > phys_memory_end) end = phys_memory_end;
    size_t p_start = (start - phys_memory_start) / PAGE_SIZE;
    size_t p_end = (end - phys_memory_start + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t p = p_start; p < p_end && p < total_pages; p++) {
        if (bit_test(p)) { bit_clear(p); used_pages--; }
    }
}

static void mark_block_used_coarse(size_t b) {
    if (!bit_test(b)) { bit_set(b); used_pages += BLOCK_SIZE; }
}
static void mark_block_free_coarse(size_t b) {
    if (bit_test(b)) { bit_clear(b); used_pages -= BLOCK_SIZE; }
}

/* Initialize PMM from a firmware/bootloader memory map.
 * 'map' is an array of phys_mem_region_t entries; entries==0 returns error.
 * Only regions with type==1 (usable) become free; all other regions are
 * treated as reserved and kept used so PMM won't allocate them.
 */
void pmm_init_from_map(const phys_mem_region_t *map, size_t entries, pmm_type_t type) {
    if (!map || entries == 0) {
        PANIC("pmm_init_from_map: invalid memory map");
    }

    current_pmm_type = type;

    /* determine overall physical range covered by the map */
    uint64_t min_start = UINT64_MAX;
    uint64_t max_end = 0;
    for (size_t i = 0; i < entries; i++) {
        if (map[i].len == 0) continue;
        if (map[i].addr < min_start) min_start = map[i].addr;
        uint64_t e = map[i].addr + map[i].len;
        if (e > max_end) max_end = e;
    }
    if (min_start == UINT64_MAX) PANIC("pmm_init_from_map: empty/invalid map");

    /* clamp start to minimum supported (1MB) to avoid low identity-map areas */
    phys_memory_start = MIN_MEMORY_START;
    if (min_start > phys_memory_start) phys_memory_start = min_start;
    phys_memory_end = max_end;
    if (phys_memory_end <= phys_memory_start) PANIC("pmm_init_from_map: no usable physical range");

    total_pages = (phys_memory_end - phys_memory_start) / PAGE_SIZE;

    /* basic sanity check and early debug print to catch format/size issues */
    if (total_pages == 0 || total_pages > (1ULL << 30)) { /* insane number of pages */
        PANIC("pmm_init_from_map: suspicious total_pages=%llu", (unsigned long long)total_pages);
    }
    LOG_INFO("PMM debug: PAGE_SIZE=%llu phys_range=0x%llx-0x%llx total_pages=%llu",
             (unsigned long long)PAGE_SIZE, (unsigned long long)phys_memory_start, (unsigned long long)phys_memory_end,
             (unsigned long long)total_pages);

    if (type == PMM_BITMAP_COARSE)
        bitmap_size_bytes = ((total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE + 7) / 8;
    else
        bitmap_size_bytes = (total_pages + 7) / 8;

    LOG_INFO("PMM debug: bitmap_size_bytes=%zu", bitmap_size_bytes);

    /* place bitmap right after the kernel for simplicity (unchanged behavior)
     * NOTE: bootloader should ensure this memory is mapped/available. */
    bitmap = (uint8_t*)(KERNEL_START + KERNEL_SIZE);
    /* mark everything used initially */
    memset(bitmap, 0xFF, bitmap_size_bytes);
    used_pages = total_pages;

    /* clear bits for usable regions only */
    if (type == PMM_BITMAP_COARSE) {
        size_t blocks = (total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE;
        for (size_t b = 0; b < blocks; b++) {
            uint64_t b_start = phys_memory_start + b * (uint64_t)BLOCK_SIZE * PAGE_SIZE;
            uint64_t b_end = b_start + (uint64_t)BLOCK_SIZE * PAGE_SIZE;
            /* mark block free only if fully contained in a usable region */
            for (size_t i = 0; i < entries; i++) {
                if (map[i].type != 1) continue;
                uint64_t r_start = map[i].addr;
                uint64_t r_end = map[i].addr + map[i].len;
                if (b_start >= r_start && b_end <= r_end) {
                    /* clear block */
                    mark_block_free_coarse(b);
                    break;
                }
            }
        }
    } else {
        /* fine-grained: clear each page that falls into a usable region */
        for (size_t i = 0; i < entries; i++) {
            if (map[i].type != 1) continue;
            uint64_t r_start = map[i].addr;
            uint64_t r_end = map[i].addr + map[i].len;
            mark_range_free_fine(r_start, r_end);
        }
    }

    /* mark kernel + bitmap area used so PMM won't hand it out */
    uint64_t r_end = KERNEL_START + KERNEL_SIZE + bitmap_size_bytes;
    if (r_end < phys_memory_start) r_end = phys_memory_start;

    if (type == PMM_BITMAP_COARSE) {
        size_t blocks = (total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE;
        for (size_t b = 0; b < blocks; b++) {
            uint64_t addr = phys_memory_start + b * (uint64_t)BLOCK_SIZE * PAGE_SIZE;
            if (addr < r_end) {
                mark_block_used_coarse(b);
            }
        }
    } else {
        mark_range_used_fine(phys_memory_start, r_end);
    }

    LOG_INFO("PMM init from map: type=%s, phys_range=0x%llx-0x%llx, total_pages=%zu, used_pages=%zu",
             pmm_get_type_name(type), (unsigned long long)phys_memory_start, (unsigned long long)phys_memory_end,
             total_pages, used_pages);
}

/* Backwards-compatible init: emulate a single usable region using the old
 * hard-coded MEMORY_START/END and warn that this is not ideal. Prefer the
 * bootloader to call pmm_init_from_map() with a real map.
 */
void pmm_init(pmm_type_t type) {
    LOG_WARN("pmm_init: no memory map provided, falling back to single-range emulation (unsafe)");
    phys_mem_region_t fake = { .addr = MIN_MEMORY_START, .len = (DEFAULT_MEMORY_END - MIN_MEMORY_START), .type = 1 };
    pmm_init_from_map(&fake, 1, type);
}

size_t pmm_get_total_memory(void) { return total_pages * PAGE_SIZE; }
size_t pmm_get_used_memory(void) { return used_pages * PAGE_SIZE; }
size_t pmm_get_free_memory(void) { return (total_pages - used_pages) * PAGE_SIZE; }

/* allocation / free */
static void *alloc_fine(void) {
    for (size_t i = 0; i < total_pages; i++) {
        if (!bit_test(i)) {
            bit_set(i);
            used_pages++;
            return (void*)(phys_memory_start + i * PAGE_SIZE);
        }
    }
    LOG_WARN("PMM: out of memory (fine)");
    return NULL;
}

static void *alloc_coarse(void) {
    size_t blocks = (total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (size_t b = 0; b < blocks; b++) {
        if (!bit_test(b)) {
            bit_set(b);
            used_pages += BLOCK_SIZE;
            return (void*)(phys_memory_start + b * BLOCK_SIZE * PAGE_SIZE);
        }
    }
    LOG_WARN("PMM: out of memory (coarse)");
    return NULL;
}

void *pmm_alloc(void) {
    return current_pmm_type == PMM_BITMAP_COARSE ? alloc_coarse() : alloc_fine();
}

static void free_fine(void *p) {
    uint64_t addr = (uint64_t)p;
    if (addr < phys_memory_start || addr >= phys_memory_end) PANIC("pmm_free: bad addr 0x%llx", addr);
    if (addr % PAGE_SIZE) PANIC("pmm_free: unaligned 0x%llx", addr);
    size_t idx = (addr - phys_memory_start) / PAGE_SIZE;
    if (!bit_test(idx)) PANIC("pmm_free: double free 0x%llx", addr);
    bit_clear(idx);
    used_pages--;
}

static void free_coarse(void *p) {
    uint64_t addr = (uint64_t)p;
    if (addr < phys_memory_start || addr >= phys_memory_end) PANIC("pmm_free: bad addr 0x%llx", addr);
    if (addr % PAGE_SIZE) PANIC("pmm_free: unaligned 0x%llx", addr);
    size_t idx = (addr - phys_memory_start) / (BLOCK_SIZE * PAGE_SIZE);
    if (!bit_test(idx)) PANIC("pmm_free: double free 0x%llx", addr);
    bit_clear(idx);
    used_pages -= BLOCK_SIZE;
}

void pmm_free(void *p) {
    if (current_pmm_type == PMM_BITMAP_COARSE) free_coarse(p); else free_fine(p);
}

void pmm_self_test(void) {
    void *a = pmm_alloc();
    if (!a) PANIC("pmm_self_test: alloc failed");
    pmm_free(a);
}

void print_pmm_metrics(void) {
    LOG_INFO("PMM metrics: alloc_calls=%llu, free_calls=%llu",
             (unsigned long long)pmm_calls_alloc, (unsigned long long)pmm_calls_free);
}

void pmm_print_memory_map(void) {
    LOG_INFO("PMM map: phys_range=0x%llx-0x%llx total=%lluKB used=%lluKB free=%lluKB",
             (unsigned long long)phys_memory_start, (unsigned long long)phys_memory_end,
             (unsigned long long)(pmm_get_total_memory()/1024), (unsigned long long)(pmm_get_used_memory()/1024), (unsigned long long)(pmm_get_free_memory()/1024));
}

/* Fetch memory map from bootloader */
size_t fetch_memory_map_from_bootloader(phys_mem_region_t *map, size_t max_entries) {
    // Placeholder: Replace this with actual bootloader integration logic.
    // For now, we simulate a memory map with two usable regions.

    if (max_entries < 2) {
        LOG_WARN("Insufficient space for memory map entries");
        return 0;
    }

    map[0].addr = 0x100000; // 1 MB
    map[0].len = 0x9F000;  // 640 KB
    map[0].type = 1;       // Usable

    map[1].addr = 0x1000000; // 16 MB
    map[1].len = 0x7F00000; // 127 MB
    map[1].type = 1;        // Usable

    return 2; // Number of entries populated
}

void kernel_main(void) {
    // Define a buffer for the memory map (adjust size as needed)
    phys_mem_region_t memory_map[16];
    size_t map_entries;

    // Fetch the memory map from the bootloader
    map_entries = fetch_memory_map_from_bootloader(memory_map, 16);

    if (map_entries == 0) {
        PANIC("Failed to fetch memory map from bootloader");
    }

    // Initialize the PMM using the fetched memory map
    pmm_init_from_map(memory_map, map_entries, PMM_BITMAP_FINE);

    // Continue with other kernel initialization tasks...
    LOG_INFO("Kernel initialization complete.");
}

