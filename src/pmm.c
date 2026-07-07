/*
 * Physical Memory Manager implementation
 *
 * Implements a bitmap-based physical frame allocator.
 *
 * Each bit in the bitmap represents one physical memory frame:
 * - 0: frame available
 * - 1: frame allocated
 *
 * Reserved frames are marked as unavailable and cannot be allocated.
 */

#include "pmm.h"
//PMM_TOTAL_FRAMES and the other constantsare declared in pmm.h
static uint8_t  pmm_bitmap[PMM_TOTAL_FRAMES / 8]; //4096 octets
static uint64_t pmm_free_count = 0;

static inline uint8_t bit_mask(uint32_t idx)
{
    return (uint8_t)(1u << (idx & 7u));
} //For a idx index frame, we retrieve the 3 least significant bits to find its position within the byte, then shift them by 1 to generate the appropriate mask.

static inline int test_bit(uint32_t idx)
{
    return (pmm_bitmap[idx >> 3] & bit_mask(idx)) != 0;
} //idx>>3 divides by 8 and returns the byte where it is located; then we use the mask to check if the frame is set to 1 = occupied.

static inline void set_bit(uint32_t idx)
{
    pmm_bitmap[idx >> 3] |= bit_mask(idx);
} //Sets the bit to 1 and leaves the others unchanged

static inline void clear_bit(uint32_t idx)
{
    pmm_bitmap[idx >> 3] &= (uint8_t)~bit_mask(idx);
} //Sets the bit to 0 and leaves the others unchanged

void pmm_init_simple(void)
{
    // Everything is free at the start
    for (uint32_t i = 0; i < (uint32_t)sizeof(pmm_bitmap); i++)
        pmm_bitmap[i] = 0;

    // Reserve 3 MiB (frames 0 to 767)
    for (uint32_t f = 0; f < (uint32_t)PMM_RESERVED_FRAMES; f++)
        set_bit(f);

    pmm_free_count = (uint64_t)PMM_TOTAL_FRAMES - (uint64_t)PMM_RESERVED_FRAMES;
}

uint64_t pmm_alloc_frame(void)
{
    for (uint32_t f = PMM_RESERVED_FRAMES; f < PMM_TOTAL_FRAMES; f++) {
        if (!test_bit(f)) {
            set_bit(f);
            pmm_free_count--; // Update the number of available frames
            return (uint64_t)f * PMM_FRAME_SIZE;
        }
    }
    return 0; // no free frame available
}

void pmm_free_frame(uint64_t phys)
{
    if ((phys & (PMM_FRAME_SIZE - 1)) != 0) return; // We check whether the address is indeed the start of a frame; its first bits must be zero since 4096 = 2^12

    uint64_t f64 = phys / PMM_FRAME_SIZE; //Since the physical address is the start of a frame, dividing it by the size of a frame gives its index
    if (f64 >= PMM_TOTAL_FRAMES) return; //The address cannot exceed the limit of our 128 MB

    uint32_t f = (uint32_t)f64;
    if (f < (uint32_t)PMM_RESERVED_FRAMES) return;  //The frame cannot occupy a kernel-reserved space

    if (test_bit(f)) {
        clear_bit(f);
        pmm_free_count++;
    }
}

uint64_t pmm_count_free(void)
{
    return pmm_free_count;
}
