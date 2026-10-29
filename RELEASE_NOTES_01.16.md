# PS3ME 01.16 — Nokia API packaging and compatibility restoration

Bounce Tales' unmodified supplied JAR now reaches the menu, Chapter 1 and
interactive gameplay in RPCS3. The missing platform classes were the blocker:
01.15 packaged neither DeviceControl, DirectUtils nor DirectGraphics. The game
references all three; adding only DeviceControl would have been incomplete.

- Compile and CLDC-preverify the Nokia UI APIs into the **packaged** runtime.
- Restore Nokia Sound through the existing PS3 MMAPI/MIDI/WAV backend. No PS2
  SPU2 code is used. Tone and OTA conversion is bounded and malformed input fails.
- Restore MMAPI `Manager.playTone`, `device://tone`, ToneControl sequences,
  sequence blocks/repeats and `audio/x-tone-seq` input streams.
- Restore upstream PS2ME's drawImage self-blit no-op compatibility behavior;
  invalid anchors still fail and drawRegion retains its existing restrictions.
- Align RGB565 fill destinations to eight bytes before 64-bit stores. Keep the
  existing wide-store path and pixel output. ASan/UBSan tests exercise the actual
  production routines across 419,152 comparisons.
- Keep 01.15's shared bounded archive snapshots, scaling optimization and all
  prior audio, controls, touchscreen, profile, resolution and RMS functionality.
- Reject incomplete runtime API packaging. Include dependency/member and native
  symbol audit evidence and a developer-only replay/capture harness.

Title ID remains `PS3ME0001`. Install over the existing app. No games, test JARs,
firmware or SDK are included in the install package. Do not delete appdb/config.

**Validation is not universal compatibility:** host and RPCS3 evidence are
documented in [the compatibility audit](docs/COMPATIBILITY_AUDIT_01.16.md).
01.16 has not been verified on physical PS3 hardware. Existing versions have
been tested on a real PS3 by Trashy12. No full-game completion is claimed.

Credits remain with the original PS2ME developers, phoneME contributors,
Trashy12, salsa64 for UI inspiration, and the existing third-party components.

