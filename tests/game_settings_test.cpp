// SPDX-License-Identifier: GPL-2.0-only
#include "GameSettings.hpp"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <unistd.h>
int main(){char temp[]="/tmp/ps3me-profiles-XXXXXX";assert(mkdtemp(temp));ps3me::GameSettings settings;ps3me::ControlConfig c;const int initial=c.map[ps3me::CROSS];int w=240,h=320;settings.begin("/games/A.jar",temp,c,w,h);std::string a=settings.base();c.map[ps3me::CROSS]=ps3me::NUM0;assert(settings.save(c,320,240));settings.end(c);assert(c.map[ps3me::CROSS]==initial);w=240;h=320;settings.begin("/games/B.jar",temp,c,w,h);assert(settings.base()!=a&&c.map[ps3me::CROSS]==initial&&w==240&&h==320);settings.end(c);w=240;h=320;settings.begin("/games/A.jar",temp,c,w,h);assert(c.map[ps3me::CROSS]==ps3me::NUM0&&w==320&&h==240);assert(settings.useGlobal(c,w,h));assert(w==240&&h==320&&c.map[ps3me::CROSS]==initial);settings.end(c);settings.begin("/games/A.jar",temp,c,w,h);assert(c.map[ps3me::CROSS]==initial);assert(!settings.save(c,-1,240));settings.end(c);std::remove((a+".controls.cfg").c_str());std::remove((a+".display.cfg").c_str());::rmdir(temp);std::puts("PASS: per-game isolation, saved resolution/mapping reload, global fallback/removal, invalid dimensions");}
