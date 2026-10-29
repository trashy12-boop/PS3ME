/* SPDX-License-Identifier: GPL-2.0-only */
#include "PointerMath.hpp"
#include <assert.h>
#include <stdio.h>

int main() {
    typedef char java_int_must_stay_32_bit[(sizeof(int32_t) == 4) ? 1 : -1];
    (void)sizeof(java_int_must_stay_32_bit);
    unsigned char buffer[256];
    assert(DERIVED(unsigned char*, buffer, 255) == buffer + 255);
    assert(DERIVED(unsigned char*, buffer + 128, -128) == buffer);
    assert(DISTANCE(buffer, buffer + 231) == 231);
    assert(DISTANCE(buffer + 231, buffer) == -231);
#if UINTPTR_MAX > UINT32_MAX
    {
        // No dereference: verify the bits that the old jint casts discarded.
        const uintptr_t base = UINT64_C(0x123456780000);
        void* high_address = (void*)base;
        assert((uintptr_t)DERIVED(void*, high_address, 0x7654) == base + 0x7654);
        assert(DISTANCE(high_address, (void*)(base + 0x1234)) == 0x1234);
        assert(DISTANCE((void*)(base + 0x1234), high_address) == -0x1234);
    }
#endif
    printf("PS3ME_VM_POINTER_MATH_OK pointer_bytes=%u\n", (unsigned)sizeof(void*));
}
