// kernel/core/pmm.c

#include "core/pmm.h"
#include "core/log.h"
#include "lib/include/libc.h"

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

static void bitmap_set(size_t page_idx) {
    bitmap[page_idx / 8] |= (1u << (page_idx % 8));
}

static void bitmap_clear(size_t page_idx) {
    bitmap[page_idx / 8] &= ~(1u << (page_idx % 8));
}

static bool bitmap_test(size_t page_idx) {
    return (bitmap[page_idx / 8] & (1u << (page_idx % 8))) != 0;
}

void pmm_init(void) {
    LOG_INFO("PMM: Initializing with fixed memory layout");
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
        if (page_idx < total_pages && bitmap_test(page_idx)) {
            bitmap_clear(page_idx);
            used_pages--;
        }
    }

    LOG_INFO("PMM: Initialized. Used pages: %u, Free pages: %u", 
             (unsigned)used_pages, (unsigned)(total_pages - used_pages));
}

void* pmm_alloc(void) {
    for (size_t i = 0; i < total_pages; ++i) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
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
    if (!bitmap_test(idx)) {
        PANIC("PMM: double free 0x%llx", addr);
    }
    bitmap_clear(idx);
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
