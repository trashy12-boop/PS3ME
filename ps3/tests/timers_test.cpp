// SPDX-License-Identifier: GPL-2.0-only
#include "CooperativeTimers.hpp"
#include <cassert>
#include <cstdio>
static int calls;
static ps3me::CooperativeTimers* backend;
static void fired(javacall_handle) { ++calls; }
static void rearm(javacall_handle handle) {
    ++calls;
    ps2::hal::Timer* t=static_cast<ps2::hal::Timer*>(handle);
    backend->disarm(t);
    t->configure(20,false,fired);
    assert(backend->arm(t));
}
int main() {
    ps3me::CooperativeTimers timers;backend=&timers;
    ps2::hal::Timer t;t.configure(10,true,fired);
    timers.poll(1000);assert(timers.arm(&t));
    timers.poll(1009);assert(calls==0);
    timers.poll(1010);assert(calls==1);
    t.suspend();timers.poll(1020);assert(calls==1);
    t.resume();timers.poll(1030);assert(calls==2);
    timers.disarm(&t);timers.poll(2000);assert(calls==2);
    t.configure(10,false,fired);assert(timers.arm(&t));
    timers.poll(2010);timers.poll(2020);assert(calls==3);
    t.configure(0,true,rearm);assert(timers.arm(&t));
    timers.poll(2021);assert(calls==4);
    timers.poll(2030);assert(calls==4);
    timers.poll(2041);assert(calls==5);
    timers.poll(3000);assert(calls==5);
    puts("PS3ME_TIMERS_OK cancellation, suspension, one-shot, callback rearm");
}
