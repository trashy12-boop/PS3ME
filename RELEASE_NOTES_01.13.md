# PS2ME PS3 01.13 — MP3 playback and repeatable sound effects

Native PowerPC/PSL1GHT build for PS3 homebrew. This update builds on 01.12,
which the user confirmed stopped the reported crashes. This new build has not
been run on a physical PS3 or RPCS3 in this session.

## Changes

- Added minimp3 (CC0) MPEG Layer III playback. Multiple independent music and
  effect Players can mix with MIDI and WAV. MP3 data stays compressed; the
  callback decodes fixed-size frames and linearly resamples to 48 kHz stereo.
  There is no whole-song PCM allocation and no allocation in the MP3 render
  callback. Encoded MP3 bytes share the 16 MiB native resident-audio budget with
  expanded WAV data. Up to 32 total native Players remain available.
- Fixed AudioPlayer.start() when the native sound has finished but Java state
  still says STARTED. Calling start() now replays that sound instead of silently
  returning. Starting an already active Player remains a no-op.
- Serialized rewind with other operations on the same Java Player. Retained
  01.12 native pool locking, WAV preflight, lazy MIDI loading, callback detachment
  on shutdown and finite-sample/clipping protection. Lazy MIDI bank initialization
  now also takes the SDL audio lock.
- Manager.getSupportedContentTypes(null) now advertises the implemented WAV,
  MIDI and MP3 input-stream formats. URL-based Player creation, AMR and AAC are
  not implemented by this release.
- Included minimp3 license in the installed application.

## Nyan Cat JAR repair — separate download

The supplied Nyan Cat v0.4 contains malformed legacy CLDC StackMap data in
Game.run(): two frames describe seven local slots while max_locals is six.
The production phoneME parser maps its detailed parsing failures to the generic
ClassFormatError 78. The classfile versions themselves are already 45.3.

Nyan_Cat_Lost_in_Space_CLDC_fixed.jar regenerates verification data with
ProGuard 6.2.2 -microedition -target 1.1, without shrinking, optimization,
obfuscation, ignored warnings, or disabling verification. Structural inspection
no longer finds the oversized frames. All 14 non-class resources, including
music, effects, graphics and the manifest, are byte-identical to the input.
Use this separate corrected JAR in place of the old Nyan Cat JAR. The emulator
still checks Java classes; it does not silently accept malformed StackMaps.
Neither the old nor repaired game is included in the PKG or source archive.

## Wonder Zoo analysis

The recovered wonder_zoo_252103.jar has ten MIDI tracks and 38 MP3 effects in
split resource 3/3.1. All 38 MP3 effects now decode to nonzero samples and pass
rewind, replay, loop and stop tests using the new engine.

The recovered v1.2.4 uses IMA ADPCM WAV effects and v1.4.6 uses PCM WAV effects.
All 76 WAV resources across those builds pass the unchanged native budget
preflight and decode to nonconstant data with host SDL2. This is not a test of
the console SDK's SDL decoder. The repeated-Player fix also applies to WAV.
We do not claim that every silent effect in every Wonder Zoo variant is fixed
without checking the user's exact variant on hardware.

## Validation actually performed

- Native PS3 ELF rebuilt with GCC 7.2 PSL1GHT; MP3 symbols present in linked ELF.
- Java AudioPlayer and Manager compiled with ECJ 3.33.0 and CLDC-preverified
  with ProGuard 6.2.2. No ignored resolver warnings.
- Host ASan/UBSan tests: all 45 MP3 assets from Bolachinha, Nyan Cat and the
  recovered Wonder Zoo MP3 variant; nonzero/finite output; deterministic rewind
  and replay; finite and infinite loops; stop; 32 simultaneous decoder slots;
  slot exhaustion/reuse; corrupt input; repeated shutdown.
- Host Java state model replaces only native entrypoints with a fake bridge:
  completed sound replay, active start no-op, stop/restart, deallocate/restart,
  closed state and MP3 format advertisement. This is not a native JVM test.
- Previous WAV budget and 960 MIDI allocation/reuse/shutdown ASan/UBSan test.
- Leak sanitizer disabled because of this host environment's restrictions.
- PKG structure, staged payload equality, version and Java ZIP CRCs checked.
- Audible output, latency, controller operation and gameplay still require PS3
  validation. Host decoder success does not establish console performance.

## Source and rebuilding

Original source attribution and licensing are retained in README.md and the
upstream/vendor directories. minimp3 source is vendored at
third_party/minimp3; origin https://github.com/lieff/minimp3 (retrieved 2026-10-29).
The exact header/license, ECJ and ProGuard hashes are recorded under
build/validation/dependency_and_repair_hashes.json.

The recovered source package contains the prebuilt VM/MIDP/platform archives
and the updated classes.zip. After obtaining the pinned SDK, the native build is:

    python3 tools/build_application.py --reuse-prebuilt-archives

To regenerate the replacement media classes using ECJ 3.33.0 and ProGuard 6.2.2:

    python3 tools/integrate_audio.py --ecj /path/to/ecj.jar --proguard /path/to/proguard.jar

Then rebuild/package using tools/package_application.py. SDK and Java build
tools are not bundled. Source package includes genuine build/test logs.
