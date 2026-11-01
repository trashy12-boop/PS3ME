// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <stdint.h>
namespace ps3me { namespace xmb {
// System overlay state gates application INPUT, not the display heartbeat.
enum Event { EventExitGame=0x0101, EventDrawBegin=0x0121,
             EventDrawEnd=0x0122, EventMenuOpen=0x0131, EventMenuClose=0x0132 };
void noteEvent(uint64_t status,uint64_t nowMicros);
bool displayHeld(uint64_t nowMicros);
uint32_t generation();
bool exitRequested();
bool takeResume();
void reset();
} }
extern "C" int ps3me_xmb_open(void);
extern "C" uint32_t ps3me_xmb_generation(void);
extern "C" int ps3me_xmb_resume(void);
extern "C" int ps3me_exit_requested(void);
extern "C" void ps3me_display_tick(void);
extern "C" int ps3me_present_blocked(void);
