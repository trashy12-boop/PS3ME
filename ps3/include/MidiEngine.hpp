// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstdint>
struct tsf;struct tml_message;
namespace ps3me {
struct MidiTrack {tsf* synth;tml_message *events,*next;uint64_t frame,end;int loops,volume;bool playing;};
class MidiEngine {tsf* bank_;int rate_;MidiTrack* tracks_[32];void reset(MidiTrack*);
public:MidiEngine();~MidiEngine();bool initialize(const char*,int);MidiTrack* open(const void*,int);void close(MidiTrack*);void start(MidiTrack*,int);void stop(MidiTrack*);void rewind(MidiTrack*);void volume(MidiTrack*,int);void render(float*,int);void shutdown();};
}
