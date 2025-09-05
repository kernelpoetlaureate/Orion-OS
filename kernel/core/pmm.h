#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

/* --- Core Public API --- */
void pmm_init(void);
void* pmm_alloc(void);
void pmm_free(void* p_addr);

/* Statistics & Testing */
size_t pmm_get_total_memory(void);
size_t pmm_get_used_memory(void);
size_t pmm_get_free_memory(void);

/* Performance measurement variables */
extern uint64_t pmm_cycles_alloc;
extern uint64_t pmm_calls_alloc;
extern uint64_t pmm_cycles_free;
extern uint64_t pmm_calls_free;

/* Declare the print_pmm_metrics function */
void print_pmm_metrics(void);

/* Help debug: run a small self-test after init */
void pmm_run_tests(void);

/* Declare the print_pmm_metrics function */
void print_pmm_metrics(void);

/* Declare the alloc_page and free_page functions */
void *alloc_page(void);
void free_page(void *page);
