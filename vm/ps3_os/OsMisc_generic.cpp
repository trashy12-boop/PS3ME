/* SPDX-License-Identifier: GPL-2.0-only */
#include "incls/_precompiled.incl"
#include "incls/_OsMisc_generic.cpp.incl"
extern "C" {
const JvmPathChar* OsMisc_get_classpath() { return NULL; }
void OsMisc_flush_icache(address start,int size) {
    if(size>0)__builtin___clear_cache((char*)start,(char*)start+size);
}
#if !defined(PRODUCT) || ENABLE_TTY_TRACE || USE_DEBUG_PRINTING
const char* OsMisc_jlong_format_specifier() { return "%lld"; }
const char* OsMisc_julong_format_specifier() { return "%llu"; }
#endif
}
