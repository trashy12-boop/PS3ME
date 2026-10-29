/* SPDX-License-Identifier: GPL-2.0-only
 * Cooperative ticker: called on VM safepoints, never from a concurrent PPU
 * thread. This avoids racing the green-thread scheduler or collector.
 */
#include "incls/_precompiled.incl"
#include "incls/_OS_generic.cpp.incl"
#include <sys/systime.h>
#include <lv2/systime.h>
extern "C" void ps3me_poll_platform() __attribute__((weak));
static bool ps3me_ticks_running=false,ps3me_ticks_suspended=false;
static u64 ps3me_tick_deadline=0;
extern "C" void ps3me_vm_poll_ticks() {
    if(!ps3me_ticks_running || ps3me_ticks_suspended)return;
    u64 now=(u64)sysGetSystemTime();
    if(now<ps3me_tick_deadline)return;
    const unsigned interval=10;
    ps3me_tick_deadline=now+(u64)interval*1000;
    if(ps3me_poll_platform)ps3me_poll_platform();
    real_time_tick(interval);
}
void ads_panic() { abort(); }
#if ENABLE_DYNAMIC_NATIVE_METHODS
void* Os::loadLibrary(const char*) { return NULL; }
void* Os::getSymbol(void*,const char*) { return NULL; }
#endif
jlong Os::java_time_millis() {
    u64 seconds=0,nanos=0;
    return sysGetCurrentTime(&seconds,&nanos)==0 ? (jlong)(seconds*1000+nanos/1000000) : 0;
}
#if SUPPORTS_MONOTONIC_CLOCK
jlong Os::monotonic_time_millis() { return sysGetSystemTime()/1000; }
#endif
void Os::sleep(jlong milliseconds) {
    if(milliseconds<0)milliseconds=0;
    while(milliseconds>0) {
        u32 slice=milliseconds>5 ? 5 : (u32)milliseconds;
        if(ps3me_poll_platform)ps3me_poll_platform();
        sysUsleep(slice*1000);milliseconds-=slice;
    }
}
jint Os::start_ticks() { ps3me_tick_deadline=(u64)sysGetSystemTime();ps3me_ticks_running=true;return true; }
void Os::stop_ticks() { ps3me_ticks_running=false; }
void Os::suspend_ticks() { ps3me_ticks_suspended=true; }
void Os::resume_ticks() { ps3me_ticks_suspended=false;ps3me_tick_deadline=(u64)sysGetSystemTime(); }
void Os::start_compiler_timer() {}
jint Os::check_compiler_timer() { return true; }
void Os::initialize() { ps3me_ticks_suspended=false;start_ticks(); }
void Os::dispose() { stop_ticks();ps3me_ticks_suspended=false; }
#if USE_HIGH_RESOLUTION_TIMER
jlong Os::elapsed_counter() { return sysGetSystemTime(); }
jlong Os::elapsed_frequency() { return 1000000; }
#endif
