#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

/* PMM types */
typedef enum {
    PMM_BITMAP_FINE,    // 1 bit per page (original)
    PMM_BITMAP_COARSE   // 1 bit per block of pages (coarse-grained)
} pmm_type_t;

/* --- Core Public API --- */
void pmm_init(pmm_type_t type);
void* pmm_alloc(void);
void pmm_free(void* p_addr);

/* Statistics & Testing */
size_t pmm_get_total_memory(void);
size_t pmm_get_used_memory(void);
size_t pmm_get_free_memory(void);

/* Get/set PMM type */
pmm_type_t pmm_get_type(void);
const char* pmm_get_type_name(pmm_type_t type);

/* Performance measurement variables */
extern uint64_t pmm_cycles_alloc;
extern uint64_t pmm_calls_alloc;
extern uint64_t pmm_cycles_free;
extern uint64_t pmm_calls_free;

/* Performance metrics */
void print_pmm_metrics(void);

/* Help debug: run a small self-test after init */
void pmm_self_test(void);
void pmm_run_tests(void);

/* Declare the print_pmm_metrics function */
void print_pmm_metrics(void);

/* Declare the alloc_page and free_page functions */
void *alloc_page(void);
void free_page(void *page);

/* Print the physical memory map */
void pmm_print_memory_map(void);
