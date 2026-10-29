// SPDX-License-Identifier: GPL-2.0-only
#include "CooperativeTimers.hpp"
namespace ps3me {
bool CooperativeTimers::arm(ps2::hal::Timer* t) {
    if(!t||!t->inUse())return false;
    disarm(t);
    for(unsigned i=0;i<8;++i) if(!slots_[i].timer) {
        Slot& s=slots_[i];s.timer=t;++s.serial;
        s.deadline=now_+static_cast<uint64_t>(t->intervalMs()>0?t->intervalMs():1);
        return true;
    }
    return false;
}
void CooperativeTimers::disarm(ps2::hal::Timer* t) {
    for(unsigned i=0;i<8;++i)if(slots_[i].timer==t){slots_[i].timer=0;++slots_[i].serial;}
}
void CooperativeTimers::poll(uint64_t now) {
    now_=now;
    if(polling_)return;
    polling_=true;
    for(unsigned i=0;i<8;++i) {
        Slot& s=slots_[i];ps2::hal::Timer* t=s.timer;
        if(!t||now<s.deadline)continue;
        const uint64_t serial=s.serial;
        s.deadline=now+static_cast<uint64_t>(t->intervalMs()>0?t->intervalMs():1);
        // A callback may cancel itself and reuse its TimerService slot.
        bool repeat=t->inUse()&&t->onFire();
        if(s.serial==serial&&!repeat){s.timer=0;++s.serial;}
    }
    polling_=false;
}
}
