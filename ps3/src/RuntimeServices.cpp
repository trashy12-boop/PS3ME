// SPDX-License-Identifier: GPL-2.0-only
#include "CooperativeTimers.hpp"
#include "Controls.hpp"
#include "hal/TimerService.hpp"
#include "hal/Keypad.hpp"
#include "hal/EventQueue.hpp"
#include "hal/IPollHook.hpp"
#include <sys/systime.h>
extern "C" void ps3me_service_app() __attribute__((weak));
namespace ps3me {
static CooperativeTimers timers;
static bool installed=false;
class PollServices: public ps2::hal::IPollHook {
public:
    void onPoll() {
        if(ps3me_service_app)ps3me_service_app();
        timers.poll(sysGetSystemTime()/1000);
        pollInput(sysGetSystemTime()/1000);
    }
};
static PollServices poller;
void installRuntimeServices() {
    timers.poll(sysGetSystemTime()/1000);
    ps2::hal::TimerService::instance().setBackend(&timers);
    ps2::hal::EventQueue::instance().setPollHook(&poller);
    installed=true;
}
}
extern "C" void ps3me_poll_platform() {
    if(ps3me::installed)ps3me::poller.onPoll();
}
