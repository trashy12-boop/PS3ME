# 01.15 — loading-path optimizations

Bounce Tales 2.0.3 was reported to remain on its loading screen. Its supplied
JAR (SHA256 189a94ec23c79c12c29b0355f6894bdd699f0f52958d7446feeef0bd1a7e6b3f)
contains eight MIDI resources and no MP3 resources. Inspection of its startup
code shows resource-batch loading and a separate loading-screen thread; the
long wait cannot simply be attributed to MP3 decoding.

## Changes

- Read-only `.jar` and `.zip` files up to 8 MiB can be snapshotted into native
  RAM, with a strict aggregate 12 MiB cap. Repeated VM reads and seeks operate on
  those bytes instead of returning to the filesystem. Larger files, exhausted
  cache budget, and allocation failures fall back to normal buffered I/O.
- Snapshots are shared by matching archive handles for one game session.
  They are retired at session boundaries; live handles remain valid until closed.
  No persistent game cache
  or new save format is introduced. Writable files, including saves, use stdio.
- RGB565 scaling uses exact quotient/remainder stepping, converts repeated
  columns once, copies repeated rows, and clears only letterbox pixels. No
  per-output-pixel division, interpolation change, frame skipping, or delay
  change is introduced.
- Runtime logs record cached archive sizes and time to the first Java frame.
  That frame can be a loading screen; it is not a loading-complete measurement.
- Incremental native linking supplies the same canonical VM file adapter before
  the recovered VM archive. The application link map is retained for checking
  that the old adapter was not linked accidentally.

## Validation

Host regression: 200 randomized, pixel-exact comparisons against the old scaler,
including up/downscaling, letterboxing and pitch padding. Random cached reads
were compared against stdio, including partial elements, EOF, seeking beyond
EOF, invalid/overflowing seeks, cache budget exhaustion and writable fallback.
The same suite passed ASan and UBSan (leak sanitizer disabled in this runtime).

`build/validation/loading_benchmark_0115.log` contains host routine timings.
They are not PS3 game loading measurements. `loading_sanitizers_0115.log`
contains the instrumented test result. No multiplier is promised for game load
speed. Native PPU compilation and package integrity checks are recorded
separately. Runtime validation status is in `loading_runtime_0115.json`.

Install the PKG over the existing app. Existing appdb/RMS and control profiles
are preserved. Games and test JARs are not included. Original PS2ME, phoneME,
Salsa64 UI inspiration, and third-party credits remain in README and licenses.
