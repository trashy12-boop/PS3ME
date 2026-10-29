# Game JAR audio findings

This records findings from the user-provided Java ME games and the native audio
changes in 01.11. Commercial JARs are not included in the source archive or
release package.

## Doodle Jump 1.8.2

The earlier JAR inspection found a cache of 27 MMAPI sound Players. Its `s`
resource contains 27 concatenated RIFF/WAVE clips: mono, unsigned 8-bit PCM at
8 kHz. The 01.08 backend reserved only 16 Player slots and 16 mixer channels,
so the remaining cached effects could fail to initialize or play. This matches
the report of some effects working while others were silent.

The game reuses Players for short effects. Rewinding a live WAV Player with
`setMediaTime(0)` previously reported success without resetting its native
channel. A retrigger before Java polled the old playback could therefore be
ignored. The backend now restarts that channel and preserves its loop count.

## Cut the Rope

The supplied JAR analysis found 11 Standard MIDI resources: five music tracks
and six effects. The existing TinySoundFont engine rendered nonzero PCM for
each effect in an isolated host test. That confirms parsing and synthesis, but
does not prove that the game schedules every effect correctly or that console
output is audible. The MIDI track pool now has 32 entries. Overloaded mixes use
a soft limiter to reduce harsh distortion when tracks and WAV channels overlap.

## Validation limits

The source test MIDlet now creates and starts 27 WAV Players at once, checks an
active WAV rewind and natural completion, then exercises a MIDI note. In this
recovered build, RPCS3 ran the older compiled smoke JAR (one WAV plus one MIDI)
and logged 226,552 nonzero samples with no limited or invalid samples. The
updated 27-player Java source still needs a fresh CLDC compile before that exact
test can run. A host test passed the limiter's pass-through range, polarity
symmetry and bounded output. RPCS3 mixer logs cannot establish speaker quality
on a PS3. The original game JARs were not present in the recovered workspace,
so their specific sound IDs still need a console check with those same JARs and
the rebuilt package.
