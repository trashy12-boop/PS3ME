/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PS3ME_HEAP_BITMAP_HPP
#define PS3ME_HEAP_BITMAP_HPP
#include <stdint.h>
#include <stddef.h>
static inline size_t ps3me_heap_slot_index(uintptr_t base, uintptr_t p, unsigned shift) {
    return (p-base) >> shift;
}
static inline size_t ps3me_bitmap_byte_offset(uintptr_t base, uintptr_t p, unsigned shift) {
    return ps3me_heap_slot_index(base,p,shift) >> 3;
}
static inline uintptr_t ps3me_heap_address_for_bitmap_byte(uintptr_t base, size_t offset, unsigned shift) {
    return base + (offset << (shift+3));
}
#endif
