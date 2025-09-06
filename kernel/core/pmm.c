// kernel/core/pmm.c

// This file implements the Physical Memory Manager (PMM) for Orion OS.
// The PMM is responsible for managing physical memory allocation and deallocation.
// It supports two types of memory management:
// 1. Fine-Grained Bitmap (PMM_BITMAP_FINE):
//    - Each bit in the bitmap represents a single page of memory.
//    - Provides precise control but requires a larger bitmap.
// 2. Coarse-Grained Bitmap (PMM_BITMAP_COARSE):
//    - Each bit in the bitmap represents a block of multiple pages (e.g., 16 or 32 pages).
//    - Reduces bitmap size but sacrifices granularity.

#include "core/pmm.h"
#include "core/log.h"
#include "lib/include/libc.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// Simple fixed memory layout for now (will be improved later)
#define MEMORY_START 0x100000    // 1MB
#define MEMORY_END   0x40000000  // 1GB (conservative estimate)
#define KERNEL_START 0x100000    // Where our kernel is loaded
#define KERNEL_SIZE  0x200000    // 2MB reserved for kernel

// Block size for coarse bitmap (16 pages = 64KB)
#define BLOCK_SIZE 32

// Internal state
static uint8_t* bitmap = NULL;
static size_t bitmap_size_bytes = 0;
static uint64_t highest_address = MEMORY_END;
static size_t total_pages = 0;
static size_t used_pages = 0;
static pmm_type_t current_pmm_type = PMM_BITMAP_FINE; // Default

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

/* Get current PMM type */
pmm_type_t pmm_get_type(void) {
    return current_pmm_type;
}

/* Get name of PMM type */
const char* pmm_get_type_name(pmm_type_t type) {
    switch (type) {
        case PMM_BITMAP_FINE:
            return "Fine-grained bitmap (1 bit per page)";
        case PMM_BITMAP_COARSE:
            return "Coarse-grained bitmap (1 bit per 16 pages)";
        default:
            return "Unknown";
    }
}

void pmm_init(pmm_type_t type) {
    LOG_INFO("PMM: Starting initialization");
    LOG_INFO("PMM: Memory range: 0x%x - 0x%x", MEMORY_START, MEMORY_END);
    LOG_INFO("PMM: Using %s", pmm_get_type_name(type));
    
    current_pmm_type = type;
    total_pages = (MEMORY_END - MEMORY_START) / PAGE_SIZE;
    
    // Calculate bitmap size based on PMM type
    if (type == PMM_BITMAP_COARSE) {
        bitmap_size_bytes = ((total_pages / BLOCK_SIZE) + 7) / 8;
    } else {
        bitmap_size_bytes = (total_pages + 7) / 8;
    }

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
    
    // Free appropriate memory regions based on PMM type
    if (type == PMM_BITMAP_COARSE) {
        for (uint64_t addr = free_start; addr < MEMORY_END; addr += BLOCK_SIZE * PAGE_SIZE) {
            size_t block_idx = (addr - MEMORY_START) / (BLOCK_SIZE * PAGE_SIZE);
            if (block_idx < total_pages / BLOCK_SIZE && bitmap_test(bitmap, block_idx)) {
                bitmap_clear(bitmap, block_idx);
                used_pages -= BLOCK_SIZE;
            }
        }
    } else {
        for (uint64_t addr = free_start; addr < MEMORY_END; addr += PAGE_SIZE) {
            size_t page_idx = (addr - MEMORY_START) / PAGE_SIZE;
            if (page_idx < total_pages && bitmap_test(bitmap, page_idx)) {
                bitmap_clear(bitmap, page_idx);
                used_pages--;
            }
        }
    }

    LOG_INFO("PMM: Initialized. Used pages: %u, Free pages: %u", 
             (unsigned)used_pages, (unsigned)(total_pages - used_pages));
    LOG_INFO("PMM: Initialization complete");
}

size_t pmm_get_total_memory(void) { return total_pages * PAGE_SIZE; }
size_t pmm_get_used_memory(void) { return used_pages * PAGE_SIZE; }
size_t pmm_get_free_memory(void) { return (total_pages - used_pages) * PAGE_SIZE; }

// Lightweight self-test
void pmm_self_test(void) {
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

/* Fine-grained bitmap allocation (1 bit per page) */
static void* pmm_alloc_fine(void)
{
    uint64_t t0 = rdtsc();
    for (size_t i = 0; i < total_pages; ++i) {
        if (!bitmap_test(bitmap, i)) {
            bitmap_set(bitmap, i);
            used_pages++;
            pmm_cycles_alloc += rdtsc() - t0;
            pmm_calls_alloc++;
            return (void*)(MEMORY_START + i * PAGE_SIZE);
        }
    }
    LOG_WARN("PMM: Out of physical memory");
    return NULL;
}

/* Coarse-grained bitmap allocation (1 bit per block of pages) */
static void* pmm_alloc_coarse(void)
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

/* Allocate a page based on current PMM type */
void* pmm_alloc(void)
{
    switch (current_pmm_type) {
        case PMM_BITMAP_FINE:
            return pmm_alloc_fine();
        case PMM_BITMAP_COARSE:
            return pmm_alloc_coarse();
        default:
            LOG_ERROR("PMM: Unknown PMM type");
            return NULL;
    }
}

/* Fine-grained bitmap free (1 bit per page) */
static void pmm_free_fine(void* page)
{
    uint64_t t0 = rdtsc();
    uint64_t addr = (uint64_t)page;
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
    pmm_cycles_free += rdtsc() - t0;
    pmm_calls_free++;
}

/* Coarse-grained bitmap free (1 bit per block of pages) */
static void pmm_free_coarse(void* page)
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

/* Free a page based on current PMM type */
void pmm_free(void* page)
{
    switch (current_pmm_type) {
        case PMM_BITMAP_FINE:
            pmm_free_fine(page);
            break;
        case PMM_BITMAP_COARSE:
            pmm_free_coarse(page);
            break;
        default:
            LOG_ERROR("PMM: Unknown PMM type");
            break;
    }
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

void pmm_print_memory_map(void) {
    static char memory_map_buffer[4096]; // Predefined buffer for memory map
    size_t offset = 0;

    offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "Physical Memory Map:\n");
    offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "Total memory: %u KB\n", (unsigned)(total_pages * PAGE_SIZE / 1024));
    offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "Used memory: %u KB\n", (unsigned)(used_pages * PAGE_SIZE / 1024));
    offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "Free memory: %u KB\n", (unsigned)((total_pages - used_pages) * PAGE_SIZE / 1024));

    offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "Bitmap (1 = used, 0 = free):\n");
    for (size_t i = 0; i < total_pages; i++) {
        offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "%c", bitmap_test(bitmap, i) ? '1' : '0');
        if ((i + 1) % 64 == 0) {
            offset += snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "\n");
        }
    }
    snprintf(memory_map_buffer + offset, sizeof(memory_map_buffer) - offset, "\n");

    // Output the buffer to the VGA or serial console for debugging
    printf("%s", memory_map_buffer);
}
