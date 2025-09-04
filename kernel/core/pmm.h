// kernel/core/pmm.h
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

/* Help debug: run a small self-test after init */
void pmm_run_tests(void);
