// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
namespace ps3me {
struct Mp3Track;
// prepare() runs on the decoder worker (256 KiB stack), never in the callback.
// attach/close/control/render are serialized with SDL's audio lock by the caller.
class Mp3Engine {
 Mp3Track* tracks_[32];
public:
 Mp3Engine(); ~Mp3Engine();
 static Mp3Track* prepare(const void*,size_t,size_t budget);
 static void discard(Mp3Track*); static size_t residentBytes(const Mp3Track*);
 bool attach(Mp3Track*);
 // Convenience for offline tests; production uses prepare() on a PS3 worker.
 Mp3Track* open(const void*,size_t); void close(Mp3Track*);
 void start(Mp3Track*,int); void stop(Mp3Track*); void rewind(Mp3Track*);
 void volume(Mp3Track*,int); bool playing(Mp3Track*) const;
 void render(float*,int); void shutdown();
};
}
