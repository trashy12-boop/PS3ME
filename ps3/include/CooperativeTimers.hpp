// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "hal/ITimerBackend.hpp"
#include "hal/Timer.hpp"
#include <stdint.h>
namespace ps3me {
// VM-owned timers: callbacks run on the Java interpreter's thread.
class CooperativeTimers: public ps2::hal::ITimerBackend {
public:
    CooperativeTimers():now_(0),polling_(false) { for(unsigned i=0;i<8;++i) slots_[i]=Slot(); }
    bool arm(ps2::hal::Timer* timer);
    void disarm(ps2::hal::Timer* timer);
    void poll(uint64_t monotonicMilliseconds);
private:
    struct Slot {
        ps2::hal::Timer* timer;
        uint64_t deadline,serial;
        Slot():timer(0),deadline(0),serial(0) {}
    } slots_[8];
    uint64_t now_;
    bool polling_;
};
void installRuntimeServices();
}
