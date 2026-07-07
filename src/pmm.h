/*
 * Physical Memory Manager interface
 *
 * Provides functions to allocate and release physical memory frames.
 *
 * The PMM manages memory using fixed-size frames tracked internally
 * by a bitmap.
 */

#pragma once
#include <stdint.h>

#define PMM_FRAME_SIZE        4096ULL //1 frame = 4KiB
#define PMM_RAM_SIZE          (128ULL * 1024ULL * 1024ULL) //128 MiB
#define PMM_TOTAL_FRAMES      (PMM_RAM_SIZE / PMM_FRAME_SIZE)

#define PMM_RESERVED_BYTES    (3ULL * 1024ULL * 1024ULL) //Reserve 3MiB
#define PMM_RESERVED_FRAMES   (PMM_RESERVED_BYTES / PMM_FRAME_SIZE) // 768

void     pmm_init_simple(void);
uint64_t pmm_alloc_frame(void);
void     pmm_free_frame(uint64_t phys);
uint64_t pmm_count_free(void);
