/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PS3ME_VM_POINTER_MATH_HPP
#define PS3ME_VM_POINTER_MATH_HPP

#include <stdint.h>

/* Native address calculations must not go through jint: Java int stays 32-bit.
 * This preserves address bits only. It does NOT widen heap reference slots,
 * stack cells, the GC bitmap or the serialized ROM representation.
 */
#define DISTANCE(low, high) (((intptr_t) (high)) - ((intptr_t) (low)))
#define DERIVED(type, ptr, offset) ((type) (((intptr_t) (ptr)) + ((intptr_t) (offset))))

#endif
