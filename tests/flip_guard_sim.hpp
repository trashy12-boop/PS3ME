// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <rsx/gcm_sys.h>
#include <cstdint>
namespace sim {
extern int64_t clk, pendingUntil;
extern bool pending, stall, injectMenuOpen;
extern int flips, waits, resets, uploads, pumps, completed;
}
extern "C" int ps3me_present_blocked(void);
extern "C" void ps3me_display_tick(void);

extern "C" void ps3me_pump_sysutil(void);
