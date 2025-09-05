// kernel/core/pmm.c

#include "core/pmm.h"
#include "core/log.h"
#include "lib/include/libc.h"
#include <stdint.h>
#include <stdio.h>

// Simple fixed memory layout for now (will be improved later)
#define MEMORY_START 0x100000    // 1MB
#define MEMORY_END   0x40000000  // 1GB (conservative estimate)
#define KERNEL_START 0x100000    // Where our kernel is loaded
#define KERNEL_SIZE  0x200000    // 2MB reserved for kernel

// Internal state
static uint8_t* bitmap = NULL;
static size_t bitmap_size_bytes = 0;
static uint64_t highest_address = MEMORY_END;
static size_t total_pages = 0;
static size_t used_pages = 0;

/* bitmap is an array of bytes (uint8_t). One bit = one page */

/* a) Mark page as used */
void bitmap_set(uint8_t *bitmap, size_t page_idx) {
    size_t byte = page_idx / 8;          /* which byte?            */
    uint8_t bit = page_idx % 8;          /* which bit inside byte? */
    bitmap[byte] |= (1u << bit);         /* set that bit to 1      */
}

/* b) Mark page as free */
void bitmap_clear(uint8_t *bitmap, size_t page_idx) {
    size_t byte = page_idx / 8;
    uint8_t bit = page_idx % 8;
    bitmap[byte] &= ~(1u << bit);        /* clear that bit to 0    */
}

/* c) Is the page used?  (returns 1 = used, 0 = free) */
int bitmap_test(uint8_t *bitmap, size_t page_idx) {
    size_t byte = page_idx / 8;
    uint8_t bit = page_idx % 8;
    return (bitmap[byte] & (1u << bit)) != 0;
}

void pmm_init(void) {
    LOG_INFO("PMM: Starting initialization");
    LOG_INFO("PMM: Memory range: 0x%x - 0x%x", MEMORY_START, MEMORY_END);

    total_pages = (MEMORY_END - MEMORY_START) / PAGE_SIZE;
    bitmap_size_bytes = (total_pages + 7) / 8;

    LOG_INFO("PMM: Total pages %u, bitmap %u bytes", (unsigned)total_pages, (unsigned)bitmap_size_bytes);

    // Place bitmap right after kernel
    bitmap = (uint8_t*)(KERNEL_START + KERNEL_SIZE);
    LOG_INFO("PMM: Placing bitmap at 0x%p", bitmap);

    // Mark all pages as used initially
    memset(bitmap, 0xFF, bitmap_size_bytes);
    used_pages = total_pages;

    // Mark usable memory as free (starting after bitmap)
    uint64_t free_start = (uint64_t)bitmap + bitmap_size_bytes;
    free_start = (free_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Align to page boundary
    
    for (uint64_t addr = free_start; addr < MEMORY_END; addr += PAGE_SIZE) {
        size_t page_idx = (addr - MEMORY_START) / PAGE_SIZE;
        if (page_idx < total_pages && bitmap_test(bitmap, page_idx)) {
            bitmap_clear(bitmap, page_idx);
            used_pages--;
        }
    }

    LOG_INFO("PMM: Initialized. Used pages: %u, Free pages: %u", 
             (unsigned)used_pages, (unsigned)(total_pages - used_pages));
    LOG_INFO("PMM: Initialization complete");
}

void* pmm_alloc(void) {
    for (size_t i = 0; i < total_pages; ++i) {
        if (!bitmap_test(bitmap, i)) {
            bitmap_set(bitmap, i);
            used_pages++;
            return (void*)(MEMORY_START + i * PAGE_SIZE);
        }
    }
    LOG_WARN("PMM: Out of physical memory");
    return NULL;
}

void pmm_free(void* p_addr) {
    uint64_t addr = (uint64_t)p_addr;
    if (addr < MEMORY_START || addr >= MEMORY_END) {
        PANIC("PMM: free out-of-range: 0x%llx", addr);
    }
    if (addr % PAGE_SIZE != 0) {
        PANIC("PMM: free non-aligned: 0x%llx", addr);
    }
    size_t idx = (addr - MEMORY_START) / PAGE_SIZE;
    if (idx >= total_pages) {
        PANIC("PMM: free index out-of-range: %u", (unsigned)idx);
    }
    if (!bitmap_test(bitmap, idx)) {
        PANIC("PMM: double free 0x%llx", addr);
    }
    bitmap_clear(bitmap, idx);
    used_pages--;
}

size_t pmm_get_total_memory(void) { return total_pages * PAGE_SIZE; }
size_t pmm_get_used_memory(void) { return used_pages * PAGE_SIZE; }
size_t pmm_get_free_memory(void) { return (total_pages - used_pages) * PAGE_SIZE; }

// Lightweight self-test
void pmm_run_tests(void) {
    LOG_INFO("PMM: running self-tests");
    size_t initial_free = pmm_get_free_memory();
    if (initial_free == 0) {
        PANIC("PMM Test: no free memory");
    }

    void* p1 = pmm_alloc();
    if (p1 == NULL) {
        PANIC("PMM Test: alloc failed");
    }

    // Try writing to allocated memory (direct access since we're in identity-mapped region)
    char *v = (char*)p1;
    v[0] = 'X'; 
    v[1] = '\0';

    pmm_free(p1);

    if (pmm_get_free_memory() != initial_free) {
        PANIC("PMM Test: free mismatch");
    }
    LOG_INFO("PMM: self-tests passed");
}

/* Variables to track performance */
uint64_t pmm_cycles_alloc = 0;
uint64_t pmm_calls_alloc = 0;
uint64_t pmm_cycles_free = 0;
uint64_t pmm_calls_free = 0;

/* x86 TSC */
static inline uint64_t rdtsc(void)
{
    unsigned hi, lo;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* Coarse-grained bitmap implementation */
#define BLOCK_SIZE 16 // Each bit represents 16 pages

/* Wrap alloc_page to measure performance */
void *alloc_page(void)
{
    uint64_t t0 = rdtsc();
    for (size_t i = 0; i < total_pages / BLOCK_SIZE; ++i) {
        if (!bitmap_test(bitmap, i)) {
            bitmap_set(bitmap, i);
            used_pages += BLOCK_SIZE;
            pmm_cycles_alloc += rdtsc() - t0;
            pmm_calls_alloc++;
            return (void*)(MEMORY_START + i * BLOCK_SIZE * PAGE_SIZE);
        }
    }
    LOG_WARN("PMM: Out of physical memory");
    return NULL;
}

/* Wrap free_page to measure performance */
void free_page(void *page)
{
    uint64_t t0 = rdtsc();
    uint64_t addr = (uint64_t)page;
    if (addr < MEMORY_START || addr >= MEMORY_END) {
        PANIC("PMM: free out-of-range: 0x%llx", addr);
    }
    if (addr % PAGE_SIZE != 0) {
        PANIC("PMM: free non-aligned: 0x%llx", addr);
    }
    size_t idx = (addr - MEMORY_START) / (BLOCK_SIZE * PAGE_SIZE);
    if (idx >= total_pages / BLOCK_SIZE) {
        PANIC("PMM: free index out-of-range: %u", (unsigned)idx);
    }
    if (!bitmap_test(bitmap, idx)) {
        PANIC("PMM: double free 0x%llx", addr);
    }
    bitmap_clear(bitmap, idx);
    used_pages -= BLOCK_SIZE;
    pmm_cycles_free += rdtsc() - t0;
    pmm_calls_free++;
}

/* Function to print performance metrics */
void print_pmm_metrics(void)
{
    printf("PMM debug: alloc_calls=%u, alloc_cycles=%u\n", (unsigned)pmm_calls_alloc, (unsigned)pmm_cycles_alloc);
    printf("PMM debug: free_calls=%u, free_cycles=%u\n", (unsigned)pmm_calls_free, (unsigned)pmm_cycles_free);
    
    if (pmm_calls_alloc > 0) {
        printf("PMM alloc: %u cycles/op\n", (unsigned)(pmm_cycles_alloc / pmm_calls_alloc));
    } else {
        printf("PMM alloc: No allocations performed\n");
    }

    if (pmm_calls_free > 0) {
        printf("PMM free: %u cycles/op\n", (unsigned)(pmm_cycles_free / pmm_calls_free));
    } else {
        printf("PMM free: No frees performed\n");
    }
}
