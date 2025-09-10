#include "core/pmm.h"
#include "core/log.h"
#include "lib/include/libc.h"
#include <stdint.h>
#include <stddef.h>

#define MIN_MEMORY_START 0x100000ULL
#define DEFAULT_MEMORY_END 0x40000000ULL
#define KERNEL_START MIN_MEMORY_START
#define KERNEL_SIZE 0x200000ULL
#define BLOCK_SIZE 32

typedef struct {
    uint8_t *bitmap;
    size_t bitmap_bytes;
    size_t total_pages;
    size_t used_pages;
    pmm_type_t type;
    uint64_t phys_start;
    uint64_t phys_end;
} PMMState;

static PMMState pmm_state = {
    .bitmap = NULL,
    .bitmap_bytes = 0,
    .total_pages = 0,
    .used_pages = 0,
    .type = PMM_BITMAP_FINE,
    .phys_start = MIN_MEMORY_START,
    .phys_end = DEFAULT_MEMORY_END
};

uint64_t pmm_cycles_alloc = 0;
uint64_t pmm_calls_alloc = 0;
uint64_t pmm_cycles_free = 0;
uint64_t pmm_calls_free = 0;

static inline void bit_set(size_t i) { pmm_state.bitmap[i >> 3] |= (1u << (i & 7)); }
static inline void bit_clear(size_t i) { pmm_state.bitmap[i >> 3] &= ~(1u << (i & 7)); }
static inline int bit_test(size_t i) { return (pmm_state.bitmap[i >> 3] >> (i & 7)) & 1; }

pmm_type_t pmm_get_type(void) { return pmm_state.type; }
const char* pmm_get_type_name(pmm_type_t t) { return t == PMM_BITMAP_COARSE ? "coarse" : "fine"; }

static void mark_range_used_fine(uint64_t start, uint64_t end) {
    if (end <= pmm_state.phys_start || start >= pmm_state.phys_end) return;
    if (start < pmm_state.phys_start) start = pmm_state.phys_start;
    if (end > pmm_state.phys_end) end = pmm_state.phys_end;
    size_t p_start = (start - pmm_state.phys_start) / PAGE_SIZE;
    size_t p_end = (end - pmm_state.phys_start + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t p = p_start; p < p_end && p < pmm_state.total_pages; p++) if (!bit_test(p)) { bit_set(p); pmm_state.used_pages++; }
}

static void mark_range_free_fine(uint64_t start, uint64_t end) {
    if (end <= pmm_state.phys_start || start >= pmm_state.phys_end) return;
    if (start < pmm_state.phys_start) start = pmm_state.phys_start;
    if (end > pmm_state.phys_end) end = pmm_state.phys_end;
    size_t p_start = (start - pmm_state.phys_start) / PAGE_SIZE;
    size_t p_end = (end - pmm_state.phys_start + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t p = p_start; p < p_end && p < pmm_state.total_pages; p++) if (bit_test(p)) { bit_clear(p); pmm_state.used_pages--; }
}

static void mark_block_used_coarse(size_t b) { if (!bit_test(b)) { bit_set(b); pmm_state.used_pages += BLOCK_SIZE; } }
static void mark_block_free_coarse(size_t b) { if (bit_test(b)) { bit_clear(b); pmm_state.used_pages -= BLOCK_SIZE; } }

void pmm_init_from_map(const phys_mem_region_t *map, size_t entries, pmm_type_t type) {
    if (!map || entries == 0) PANIC("pmm_init_from_map: invalid memory map");
    pmm_state.type = type;
    uint64_t min_start = UINT64_MAX; uint64_t max_end = 0;
    for (size_t i = 0; i < entries; i++) { if (map[i].len == 0) continue; if (map[i].addr < min_start) min_start = map[i].addr; uint64_t e = map[i].addr + map[i].len; if (e > max_end) max_end = e; }
    if (min_start == UINT64_MAX) PANIC("pmm_init_from_map: empty/invalid map");
    pmm_state.phys_start = MIN_MEMORY_START; if (min_start > pmm_state.phys_start) pmm_state.phys_start = min_start;
    pmm_state.phys_end = max_end; if (pmm_state.phys_end <= pmm_state.phys_start) PANIC("pmm_init_from_map: no usable physical range");
    pmm_state.total_pages = (pmm_state.phys_end - pmm_state.phys_start) / PAGE_SIZE;
    if (pmm_state.total_pages == 0 || pmm_state.total_pages > (1ULL << 30)) PANIC("pmm_init_from_map: suspicious total_pages=%llu", (unsigned long long)pmm_state.total_pages);
    if (type == PMM_BITMAP_COARSE) pmm_state.bitmap_bytes = ((pmm_state.total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE + 7) / 8; else pmm_state.bitmap_bytes = (pmm_state.total_pages + 7) / 8;
    pmm_state.bitmap = (uint8_t*)(KERNEL_START + KERNEL_SIZE);
    memset(pmm_state.bitmap, 0xFF, pmm_state.bitmap_bytes);
    pmm_state.used_pages = pmm_state.total_pages;
    if (type == PMM_BITMAP_COARSE) {
        size_t blocks = (pmm_state.total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE;
        for (size_t b = 0; b < blocks; b++) {
            uint64_t b_start = pmm_state.phys_start + b * (uint64_t)BLOCK_SIZE * PAGE_SIZE;
            uint64_t b_end = b_start + (uint64_t)BLOCK_SIZE * PAGE_SIZE;
            for (size_t i = 0; i < entries; i++) {
                if (map[i].type != 1) continue;
                uint64_t r_start = map[i].addr; uint64_t r_end = map[i].addr + map[i].len;
                if (b_start >= r_start && b_end <= r_end) { mark_block_free_coarse(b); break; }
            }
        }
    } else {
        for (size_t i = 0; i < entries; i++) if (map[i].type == 1) mark_range_free_fine(map[i].addr, map[i].addr + map[i].len);
    }
    uint64_t r_end = KERNEL_START + KERNEL_SIZE + pmm_state.bitmap_bytes; if (r_end < pmm_state.phys_start) r_end = pmm_state.phys_start;
    if (type == PMM_BITMAP_COARSE) {
        size_t blocks = (pmm_state.total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE;
        for (size_t b = 0; b < blocks; b++) { uint64_t addr = pmm_state.phys_start + b * (uint64_t)BLOCK_SIZE * PAGE_SIZE; if (addr < r_end) mark_block_used_coarse(b); }
    } else mark_range_used_fine(pmm_state.phys_start, r_end);
}

void pmm_init(pmm_type_t type) {
    phys_mem_region_t fake = { .addr = MIN_MEMORY_START, .len = (DEFAULT_MEMORY_END - MIN_MEMORY_START), .type = 1 };
    pmm_init_from_map(&fake, 1, type);
}

size_t pmm_get_total_memory(void) { return pmm_state.total_pages * PAGE_SIZE; }
size_t pmm_get_used_memory(void) { return pmm_state.used_pages * PAGE_SIZE; }
size_t pmm_get_free_memory(void) { return (pmm_state.total_pages - pmm_state.used_pages) * PAGE_SIZE; }

static void *alloc_fine(void) { for (size_t i = 0; i < pmm_state.total_pages; i++) if (!bit_test(i)) { bit_set(i); pmm_state.used_pages++; return (void*)(pmm_state.phys_start + i * PAGE_SIZE); } return NULL; }
static void *alloc_coarse(void) { size_t blocks = (pmm_state.total_pages + BLOCK_SIZE - 1) / BLOCK_SIZE; for (size_t b = 0; b < blocks; b++) if (!bit_test(b)) { bit_set(b); pmm_state.used_pages += BLOCK_SIZE; return (void*)(pmm_state.phys_start + b * BLOCK_SIZE * PAGE_SIZE); } return NULL; }

void *pmm_alloc(void) { return pmm_state.type == PMM_BITMAP_COARSE ? alloc_coarse() : alloc_fine(); }

static void free_fine(void *p) { uint64_t addr = (uint64_t)p; if (addr < pmm_state.phys_start || addr >= pmm_state.phys_end) PANIC("pmm_free: bad addr 0x%llx", addr); if (addr % PAGE_SIZE) PANIC("pmm_free: unaligned 0x%llx", addr); size_t idx = (addr - pmm_state.phys_start) / PAGE_SIZE; if (!bit_test(idx)) PANIC("pmm_free: double free 0x%llx", addr); bit_clear(idx); pmm_state.used_pages--; }
static void free_coarse(void *p) { uint64_t addr = (uint64_t)p; if (addr < pmm_state.phys_start || addr >= pmm_state.phys_end) PANIC("pmm_free: bad addr 0x%llx", addr); if (addr % PAGE_SIZE) PANIC("pmm_free: unaligned 0x%llx", addr); size_t idx = (addr - pmm_state.phys_start) / (BLOCK_SIZE * PAGE_SIZE); if (!bit_test(idx)) PANIC("pmm_free: double free 0x%llx", addr); bit_clear(idx); pmm_state.used_pages -= BLOCK_SIZE; }

void pmm_free(void *p) { if (pmm_state.type == PMM_BITMAP_COARSE) free_coarse(p); else free_fine(p); }

void pmm_self_test(void) { void *a = pmm_alloc(); if (!a) PANIC("pmm_self_test: alloc failed"); pmm_free(a); }

void print_pmm_metrics(void) { }
void pmm_print_memory_map(void) { }


