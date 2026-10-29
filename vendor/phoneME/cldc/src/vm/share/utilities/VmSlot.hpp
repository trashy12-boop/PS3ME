/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PS3ME_VM_SLOT_HPP
#define PS3ME_VM_SLOT_HPP
#include <stdint.h>
#include <string.h>
typedef uintptr_t ps3me_vm_slot_t;
static inline ps3me_vm_slot_t ps3me_slot_read_word(const void* p) {
    ps3me_vm_slot_t value; memcpy(&value,p,sizeof(value)); return value;
}
static inline int32_t ps3me_slot_read_int(const void* p) {
    return (int32_t)ps3me_slot_read_word(p);
}
static inline void ps3me_slot_write_word(void* p, ps3me_vm_slot_t value) {
    memcpy(p,&value,sizeof(value));
}
static inline void ps3me_slot_write_int(void* p, int32_t value) {
    ps3me_slot_write_word(p,(ps3me_vm_slot_t)(intptr_t)value);
}
#endif
