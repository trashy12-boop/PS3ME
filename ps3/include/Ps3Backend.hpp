// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Storage.hpp"
#include "hal/IFramebuffer.hpp"
#include "hal/IPad.hpp"
#include "hal/IClock.hpp"
#include "hal/IHeap.hpp"
#include "hal/IOutputSink.hpp"
#include <SDL.h>
#include <stdint.h>

namespace ps3me {
class Framebuffer : public ps2::hal::IFramebuffer {
public:
    Framebuffer();
    ~Framebuffer();
    bool initializeVideo();
    javacall_pixel* map(int width,int height);
    void unmap();
    void present();
    void presentRegion(int first,int last);
    void refreshCursor();
private:
    enum { MAX_WIDTH=1024,MAX_HEIGHT=1024 };
    javacall_pixel* raster_;
    SDL_Surface *screen_,*converted_;
    int width_,height_;
    bool haveFrame_;int paintedX_,paintedY_;bool paintedDown_;uint64_t lastFlip_;
    void blitCursor();
};
class Controller : public ps2::hal::IPad {
public:
    Controller():initialized_(false),port_(-1) {}
    ~Controller();
    bool ensureReady();
    bool read(ps2::hal::PadButtons* out);
private:
    bool initialized_;
    int port_;
    ps2::hal::PadButtons last_;
};
class Clock : public ps2::hal::IClock {
public:
    javacall_int64 wallClockMillis();
    javacall_int64 monotonicCounter();
    javacall_int64 monotonicFrequency();
    void sleep(javacall_uint64 milliseconds);
};
class Heap : public ps2::hal::IHeap {
public:
    void* allocate(unsigned int size);
    void* reallocate(void* ptr,unsigned int size);
    void deallocate(void* ptr);
};
class Log : public ps2::hal::IOutputSink {
public:
    Log():file_(0) {}
    ~Log();
    bool open(const char* path);
    void write(const char* data,int length);
private:
    FILE* file_;
};
// PCM output is native; synthesis/MMAPI wiring remains separate VM work.
class Audio {
public:
    Audio();
    ~Audio();
    bool initialize(int sampleRate);
    unsigned int enqueue(const int16_t* interleavedStereo,unsigned int frames);
private:
    enum { CAPACITY=32768 }; // stereo frames
    int16_t samples_[CAPACITY*2];
    unsigned int head_,count_;
    bool open_;
    static void callback(void* self,Uint8* bytes,int length);
};
// Own these objects for at least as long as the MIDP/JavaCall runtime.
// Does not start a VM and intentionally does not pretend to provide its scheduler.
void installPlatform(Storage& storage,Framebuffer& framebuffer,Controller& controller,
                     Clock& clock,Heap& heap,Log& log);
}
