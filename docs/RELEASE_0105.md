# Reconstructed 01.05 validation

The installed EBOOT.BIN (SHA256 743b662bf2e9f86dd17d384921b61224c03daab980dec7ffb2da285e134ed5dc)
was tested with RPCS3 0.0.43-20412 / firmware 4.93, PPU static interpreter, Null renderer.
The sequence opened TouchSmoke, AudioSmoke, TouchSmoke, then MidiOverlapSmoke.
All four sessions finalized; the WAV/MIDI smoke and concurrent Cut the Rope MIDI
music/effect test passed. Menu reached 120 frames. No fatal emulator error or
uncaught Java launch failure was observed. Forced Start exits return -1; the
self-terminating overlap MIDlet returns 0. Generic Diagnostic-specific booleans
are false because this test intentionally substitutes our overlap MIDlet for
Diagnostic. See summary.json and the retained runtime logs.

Host ASAN/UBSAN tests exercise independent players, close, rewind, mute and finite
PCM using the attached game's MIDI resources. Commercial resources, fixture JAR,
emulator and firmware are excluded from this source archive. The package contains
only our original three diagnostic JARs. Package payloads were byte-compared with
the files installed by RPCS3. The installer CLI aborts during teardown after a
successful install; the installed file comparison passed.

The software menu surface was inspected: Cut the Rope's original embedded PNG,
its manifest title/vendor, readable layout and footer were visible. This does not
prove television/RSX output or physical controller behavior. No new hardware
compatibility or subjective audio-quality claim is made.

The canonical materialized executable for packaging is build/ps3/release_native.elf.
Build tools can expose incomplete mmap-written intermediate files in this hosted
filesystem, so the build copies the verified payload through ordinary writes.
The canonical executable and source checksums are retained in the source ZIP.

The reconstructed browser caches JAR titles/icons in RAM only; Select clears the
cache and rescans. A persistent Salsa64-style disk listing cache is not included.
