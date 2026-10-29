# PS2ME PS3 01.14: MP3 stack overflow repair

In 01.13, MP3 decoding occurred inside SDL's audio callback. The pinned PS3 SDK gives SDL threads a 16,384-byte stack, while the PPU compiler reports a 17,040-byte stack frame for `mp3dec_decode_frame()`. A host playback test reproduced a crash with a guarded 16 KiB stack.

## Changes

- MP3 preparation now runs on a joinable PPU worker with an explicit 256 KiB stack, outside the SDL audio callback.
- Render callback only mixes prepared PCM, with no decoding, allocations, waits, or file access.
- Bounded 16 MiB shared native audio budget, including resident audio storage.
- Source-rate/channel signed 16-bit PCM avoids expanded 48 kHz stereo float storage.
- Integer sample phase allows deterministic looping and replay.
- Retains MIDI, WAV, prior Player replay fixes, and 32 native Player slots.

## Validation

- Host-based guarded-stack test and PPU compiler stack audit.
- ASan/UBSan host tests for 45 MP3 assets, replay, looping, slots, errors, and budgets.
- PowerPC64 Cell LV2 ELF rebuilt and package payload structurally validated.
- **No PS3 hardware or RPCS3 execution of 01.14 has been performed.** The host tests are not proof of console compatibility.

## Installing

Install `PS2ME_PS3_01.14.pkg` over 01.13 with HEN enabled without deleting the existing app/saved data. No game JARs are bundled. Nyan Cat requires the separately corrected JAR supplied with 01.13.

The full original source archive retains its separate licensing, attribution and dependency notices.

**Note:** The GitHub repository import is currently partial: do not assume these individual source files alone constitute a buildable tree.
