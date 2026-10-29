// SPDX-License-Identifier: GPL-2.0-only
// A developer link check, NOT a Java emulator or a distributable game launcher.
#include "Ps3Backend.hpp"
#include <javacall_keypress.h>
#include <javacall_penevent.h>
#include <cstdlib>
// The integration test observes this VM boundary without linking a Java engine.
// This symbol exists ONLY in this test translation unit.
extern "C" void javanotify_key_event(javacall_key, javacall_keypress_type) {}
extern "C" void javanotify_pen_event(int,int,javacall_penevent_type) {}
int main() {
    ps3me::Framebuffer video;
    if(!video.initializeVideo()||!video.map(240,320))return EXIT_FAILURE;
    ps3me::Controller pad;pad.ensureReady();
    ps2::hal::PadButtons state;pad.read(&state);
    ps3me::Clock clock;
    ps3me::Heap heap;
    ps3me::Log log;
    ps3me::Storage storage(std::vector<std::string>{"/dev_hdd0/game/PS3ME0001/USRDIR/games","/dev_usb000/PS3ME/games"});
    ps3me::installPlatform(storage,video,pad,clock,heap,log);
    storage.list();
    ps3me::Audio audio;
    if(audio.initialize(44100)) { int16_t silence[32]={0};audio.enqueue(silence,16); }
    if(clock.monotonicCounter()<0)return EXIT_FAILURE;
    video.present();
    return EXIT_SUCCESS;
}
