/*
 * Virtual Memory Manager interface
 *
 * Provides functions to create and manage virtual memory mappings
 * using x86-64 paging structures.
 *
 * Supports page mapping, address translation, and permission management
 * through page table flags.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include "pmm.h"

#define ENTRIES_PER_TABLE 512
// Page table flags are stored in the least significant bits
#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_RW       (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_GLOBAL   (1ULL << 8)

#define MAX_KERNEL_PTS 64  // Maximum number of PTs supported

// --- Global tables ---
extern uint64_t *pml4;

//Faster flag declaration
#define KERNEL_FLAGS (PAGE_PRESENT | PAGE_RW | PAGE_GLOBAL)
#define USER_FLAGS   (PAGE_PRESENT | PAGE_RW | PAGE_USER)

typedef uint64_t pte_t;
typedef pte_t*   pt_t;   // Pointer to a 4 KiB page table containing 512 entries

//Allocates a frame for a table and initializes it to 0
uint64_t alloc_table_phys(void);

//Creation of a page entry
uint64_t make_pte(uint64_t phys_addr, uint64_t flags);

//Add a page mapping to a page table entry
void map_page(uint64_t *pt, size_t index, uint64_t phys_addr, uint64_t flags);

//Update the permission flags of an existing virtual page mapping
int vmm_set_flags(uint64_t virt_addr, uint64_t new_flags);
//Identity mapping: virtual address is mapped to the same physical address
int vmm_map_page(uint64_t virt_addr, uint64_t flags);
//Retrieve mapping information for a virtual address
int vmm_get_flags(uint64_t virt_addr, uint64_t *out_flags);
int vmm_get_phys(uint64_t virt_addr, uint64_t *out_phys);

//Initializes paging and jumps to the kernel
__attribute__((noreturn))
void init_paging_and_jump(void (*target)(void));
