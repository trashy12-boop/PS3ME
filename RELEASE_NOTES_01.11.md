# PS2ME for PS3 01.11

This release expands the native audio pool to 32 Players and mixer channels,
matching the 27-player WAV cache identified in Doodle Jump 1.8.2. Rewinding an
active WAV Player now restarts its clip with the configured loop count. The
TinySoundFont MIDI track pool also has 32 slots, and the final mix uses a soft
limiter to reduce harsh clipping when music and effects overlap.

The release package contains no internal diagnostic JARs, commercial game JARs
or user saves. It targets big-endian PowerPC64/Cell. Host and RPCS3 smoke checks
are not a substitute for testing audio output on a physical PS3.
