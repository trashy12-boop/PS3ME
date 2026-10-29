/* SPDX-License-Identifier: GPL-2.0-only */
#include "incls/_precompiled.incl"
#include "incls/_OsMemory_generic.cpp.incl"
#include <stdlib.h>
extern "C" void* OsMemory_allocate(size_t bytes) { return malloc(bytes); }
extern "C" void OsMemory_free(void* address) { free(address); }
