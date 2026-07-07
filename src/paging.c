/*
 * Virtual Memory Manager implementation
 *
 * Implements x86-64 four-level paging:
 * PML4 -> PDPT -> PD -> PT
 *
 * This module creates page tables, maps virtual addresses to
 * physical frames, manages page permissions, and enables paging.
 *
 * The current implementation uses identity mapping for kernel pages.
 */

#include <stdint.h>
#include "paging.h"
#include "pmm.h"

// Pointers to the paging tables, initialized later
uint64_t *pml4;

// No printf available yet; used to log test results after paging init
extern void serial_puts(const char*);
extern void serial_hex64(uint64_t);

// Allocates a table (PML4/PDPT/PD/PT), zeroes its 512 entries, returns its physical address
uint64_t alloc_table_phys(void) {
    serial_puts("[PAGING] Allocating table frame...\n");

    uint64_t phys = pmm_alloc_frame();
    if (!phys) return 0; // allocation failed

    uint64_t *table = (uint64_t *)phys; // treat the physical address as a table
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) // Initialize all page table entries as not present
        table[i] = 0;

    serial_puts("[PAGING] Table allocated at ");
    serial_hex64(phys);
    serial_puts("\n");

    return phys;
}

// Builds a page table entry from a physical address and flags
uint64_t make_pte(uint64_t phys_addr, uint64_t flags) {
    return (phys_addr & 0x000FFFFFFFFFF000ULL) | flags;
}

// Writes one entry (phys_addr + flags) into a given page table at index
void map_page(uint64_t *pt_entry, size_t index, uint64_t phys_addr, uint64_t flags) {
    pt_entry[index] = make_pte(phys_addr, flags);
}

// Changes the flags of an existing mapping at virt_addr
int vmm_set_flags(uint64_t virt_addr, uint64_t new_flags) {
    if ((virt_addr & ~0xFFFULL) >= PMM_RAM_SIZE)
        return -1;
    // Bits 12-51 of the virtual address give the index into each table level;
    // mask with 0x1FF (9 bits) after shifting by 39/30/21/12
    size_t pml4_i = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_i = (virt_addr >> 30) & 0x1FF;
    size_t pd_i   = (virt_addr >> 21) & 0x1FF;
    size_t pt_i   = (virt_addr >> 12) & 0x1FF;

    // Walk pml4 -> pdpt -> pd -> pt, checking PAGE_PRESENT at each level
    if (!(pml4[pml4_i] & PAGE_PRESENT)) return -1;
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return -1;
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) return -1;
    uint64_t *pt = (uint64_t *)(pd[pd_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pt[pt_i] & PAGE_PRESENT)) return -1;
    
    // Current flags
    uint64_t old_flags = pt[pt_i] & 0xFFFULL;

    // Refuse to touch non-USER (kernel) pages
    if (!(old_flags & PAGE_USER)) return -1;

    // Update flags, keeping PAGE_PRESENT set (identity mapping: virt == phys)
    uint64_t phys = pt[pt_i] & 0x000FFFFFFFFFF000ULL;
    pt[pt_i] = phys | (new_flags | PAGE_PRESENT);
    
    // Force the CPU to pick up the flag change immediately
    asm volatile("invlpg (%0)" :: "r"(virt_addr & ~0xFFFULL) : "memory");
    serial_puts("[vmm] TLB invalidated\n");
    return 0;
}

// Maps a user page (identity mapping: virt == phys); same walk as vmm_set_flags
// but creates missing tables and sets the physical address too
int vmm_map_page(uint64_t virt_addr, uint64_t flags) {
    serial_puts("[vmm] Mapping page... Virtual address:");
    serial_hex64(virt_addr & ~0xFFFULL);
    serial_puts("\n");

    // Reject out-of-range addresses
    if ((virt_addr & ~0xFFFULL) >= PMM_RAM_SIZE - 1) {
        serial_puts("[vmm] ERROR: Address out of range\n");
        return -1;
    }
    
    // Table indices from the virtual address
    size_t pml4_i = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_i = (virt_addr >> 30) & 0x1FF;
    size_t pd_i   = (virt_addr >> 21) & 0x1FF;
    size_t pt_i   = (virt_addr >> 12) & 0x1FF;
    
    // Allocate PDPT/PD/PT on demand if missing
    if (!(pml4[pml4_i] & PAGE_PRESENT)){
        serial_puts("[vmm] PML4 entry not present, allocating PDPT...\n");
        uint64_t phys = alloc_table_phys();
        if (!phys) {
            serial_puts("[vmm] ERROR: Failed to allocate PDPT\n");
            return -1;
        }
        pml4[pml4_i] = phys | KERNEL_FLAGS;
        serial_puts("[vmm] PDPT allocated at ");
        serial_hex64(phys);
        serial_puts("\n");
    }
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)){
        serial_puts("[vmm] PDPT entry not present, allocating PD...\n");
        uint64_t phys = alloc_table_phys();
        if (!phys) {
            serial_puts("[vmm] ERROR: Failed to allocate PD\n");
            return -1;
        }
        pdpt[pdpt_i] = phys | KERNEL_FLAGS;
        serial_puts("[vmm] PD allocated at ");
        serial_hex64(phys);
        serial_puts("\n");
    }
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_i] & PAGE_PRESENT)){
        serial_puts("[vmm] PD entry not present, allocating PT...\n");
        uint64_t phys = alloc_table_phys();
        if (!phys) {
            serial_puts("[vmm] ERROR: Failed to allocate PT\n");
            return -1;
        }
        pd[pd_i] = phys | KERNEL_FLAGS;
        serial_puts("[vmm] PT allocated at ");
        serial_hex64(phys);
        serial_puts("\n");
    }
    uint64_t *pt = (uint64_t *)(pd[pd_i] & 0x000FFFFFFFFFF000ULL);

    uint64_t old_flags = pt[pt_i] & 0xFFFULL;
    if (!(old_flags & PAGE_USER) && (old_flags & PAGE_PRESENT)) {
        serial_puts("[vmm] ERROR: Trying to overwrite kernel page!\n");
        return -1;
    }
    
    // Map the new address with the given flags (plus PAGE_PRESENT)
    pt[pt_i] = (virt_addr & 0x000FFFFFFFFFF000ULL) | (flags | PAGE_PRESENT);

    serial_puts("[vmm] Page mapped successfully\n");
    
    // Invalidate the TLB for this page
    asm volatile("invlpg (%0)" :: "r"(virt_addr & ~0xFFFULL) : "memory");
    serial_puts("[vmm] TLB invalidated\n");
    return 0;
}

// Reads back the flags of a mapped address (reverse of vmm_map_page)
int vmm_get_flags(uint64_t virt_addr, uint64_t *out_flags) {
    size_t pml4_i = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_i = (virt_addr >> 30) & 0x1FF;
    size_t pd_i   = (virt_addr >> 21) & 0x1FF;
    size_t pt_i   = (virt_addr >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) return -1;
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return -1;
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) return -1;
    uint64_t *pt = (uint64_t *)(pd[pd_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pt[pt_i] & PAGE_PRESENT)) return -1;

    *out_flags = pt[pt_i] & 0xFFFULL;
    return 0;
}

// Reads back the mapped physical address (same walk as above)
int vmm_get_phys(uint64_t virt_addr, uint64_t *out_phys) {
    size_t pml4_i = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_i = (virt_addr >> 30) & 0x1FF;
    size_t pd_i   = (virt_addr >> 21) & 0x1FF;
    size_t pt_i   = (virt_addr >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) return -1;
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return -1;
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) return -1;
    uint64_t *pt = (uint64_t *)(pd[pd_i] & 0x000FFFFFFFFFF000ULL);

    if (!(pt[pt_i] & PAGE_PRESENT)) return -1;

    *out_phys = pt[pt_i] & 0x000FFFFFFFFFF000ULL;
    return 0;
}

// Sets up paging and jumps into the kernel
__attribute__((noreturn))
void init_paging_and_jump(void (*target)(void)) {
    serial_puts("[PAGING] Initializing paging...\n");

    // Get the first available physical frame after the loaded kernel
    uint64_t first_free_add = pmm_alloc_frame();

    // Allocate the top-level PML4 / PDPT / PD tables via the PMM
    uint64_t pml4_phys = alloc_table_phys();
    uint64_t pdpt_phys = alloc_table_phys();
    uint64_t pd_phys   = alloc_table_phys();

    if(pml4_phys == 0 || pdpt_phys == 0 || pd_phys == 0) {
        serial_puts("[PANIC] Failed to allocate paging tables!\n");
        for (;;) __asm__ volatile("hlt");
    }

    // Global pointers to the tables; PTs are allocated later
    // Current implementation assumes 128 MiB of physical memory.
    // A larger memory range would require additional PDPT/PD entries.
    pml4 = (uint64_t *)pml4_phys;
    uint64_t *pdpt = (uint64_t *)pdpt_phys;
    uint64_t *pd   = (uint64_t *)pd_phys;

    // Map all physical frames occupied by the kernel, plus a safety margin
    size_t kernel_frames = (first_free_add/1024)+100;
    // Number of PTs needed to cover all kernel frames
    size_t num_pt = (kernel_frames + ENTRIES_PER_TABLE - 1) / ENTRIES_PER_TABLE;

    if (num_pt > MAX_KERNEL_PTS) {
        serial_puts("[PANIC] Too many PTs!\n");
        for (;;) __asm__ volatile("hlt");
    }

    uint64_t *pt[MAX_KERNEL_PTS];
    serial_puts("[PAGING] Allocating PTs...\n");
    // Allocate each PT needed for the kernel
    for (size_t i = 0; i < num_pt; i++) {
        uint64_t pt_phys_i = alloc_table_phys();
        if (pt_phys_i == 0) {
            serial_puts("[PANIC] Failed to allocate PT!\n");
            for (;;) __asm__ volatile("hlt");
        }
        pt[i] = (uint64_t *)pt_phys_i;
    }

    // Link PML4 -> PDPT -> PD -> PT
    pml4[0] = make_pte(pdpt_phys, KERNEL_FLAGS);
    pdpt[0] = make_pte(pd_phys, KERNEL_FLAGS);
    for (size_t i = 0; i < num_pt; i++) {
        pd[i] = make_pte((uint64_t)pt[i], KERNEL_FLAGS);
    }

    // Map kernel frames (physical address + kernel flags)
    serial_puts("[PAGING] Mapping kernel frames...\n");
    for (size_t frame = 0; frame < kernel_frames; frame++) {
        size_t pt_index = frame % ENTRIES_PER_TABLE;
        size_t pd_index = frame / ENTRIES_PER_TABLE;
        map_page(pt[pd_index], pt_index, frame * PMM_FRAME_SIZE, KERNEL_FLAGS);
    }
    serial_puts("[PAGING] Kernel frames mapped\n");

    // Enable paging: CR3 points to PML4
    serial_puts("[PAGING] Setting CR3...\n");
    asm volatile("mov %0, %%cr3" :: "r"(pml4_phys));

    serial_puts("[PAGING] Enabling paging (CR0)...\n");
    // Set CR0 bit 31 to enable paging
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // enable paging
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    serial_puts("[PAGING] Paging enabled\n");

    uint64_t free0 = pmm_count_free();
    serial_puts("[PMM] free after init = ");
    serial_hex64(free0);
    serial_puts("\n");

    // Quick smoke test of the flag-editing functions
    serial_puts("[PAGING] Testing vmm_map_page...\n");
    vmm_map_page(0x07FF0000, USER_FLAGS); // map a user page at 0x07FF0000
    serial_puts("[PAGING] Mapped user page at 0x07FF0000\n");
    uint64_t flags, phys;
    if (vmm_get_flags(0x07FF0000, &flags) == 0) {
        serial_puts("Flags: ");
        serial_hex64(flags);
        serial_puts("\n");
    }
    if (vmm_get_phys(0x07FF0000, &phys) == 0) {
        serial_puts("Phys: ");
        serial_hex64(phys);
        serial_puts("\n");
    }
    vmm_set_flags(0x07FF0000, USER_FLAGS | PAGE_RW); // add write access to that page
    
    serial_puts("[PAGING] Jumping to kernel_main...\n");
    
    target();
    __builtin_unreachable();
}