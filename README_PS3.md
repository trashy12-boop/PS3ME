# PS2ME PS3 01.14

This replaces 01.13's unsafe MP3 audio-thread decoder. MP3 preparation now runs
on a dedicated 256 KiB PS3 worker; playback only mixes bounded source-rate
16-bit PCM. The pinned SDK's 16 KiB SDL audio thread cannot accommodate the
17,040-byte MP3 decoder frame used in 01.13.

Enable HEN and install PS2ME_PS3_01.14.pkg over the current version. Keep your
existing Bolachinha JAR. No games/test JARs are bundled. Games, RMS data and
profiles remain in their existing directories. For Nyan Cat, use the separate
corrected CLDC JAR previously supplied.

Host stack-overflow regression, playback and memory-budget tests passed.
This version is compiled/packaged but has not been run on a PS3 or RPCS3.
See RELEASE_NOTES_01.14.md for changes and validation scope.

## In-game menu

Press both stick buttons, **L3 + R3**, while a game is running. The native menu
suspends Java execution and audio output while open. Release both stick buttons
before opening it again. The combination is reserved even if either stick button
has a phone-key mapping; individual stick clicks can still be remapped.

- Up/Down selects an entry. Left/Right or X changes a value.
- Game resolution: 176x220, 240x320, 320x240, 320x480, 352x416, 480x800.
- Button mappings: arrows, Fire, softkeys, 0–9, star/hash, touch/hold or disabled.
- Cursor enable, speed and stick deadzone can also be changed.
- Circle or Start resumes with the current settings for this session.
- Save settings for this game writes its controls AND selected resolution.
- Use global defaults removes this game's saved overrides and restores its
  inherited mappings and the resolution selected in the library before launch.
- Resume game returns to play; Return to game library ends the Java session.

Start outside this menu still returns directly to the game library. Square in
the library still edits GLOBAL control defaults. Existing global controls.cfg
is preserved. Opening a game loads its override if present; returning to the
library restores the global controls in memory.


## Landscape and game compatibility

320x240 is landscape; 240x320 is portrait (width x height). The menu sends a
real MIDP size-change event and updates the touch coordinate bounds when it
resumes. Some games use fixed graphics/layouts or explicitly reject landscape.
Those need a matching 320x240 edition, or a restart after saving the new size.
The emulator cannot make every portrait-only JAR support landscape.

Java execution is suspended while the menu is open, but system wall-clock time
continues. A game that computes large time deltas on resume may catch up.

## Files and saves

Base: /dev_hdd0/game/PS3ME0001/USRDIR

- games: JAR/JAD files; HDD and USB browsing remain available.
- appdb: installed suites and RMS saves, unchanged by this update.
- config/controls.cfg: global controls used by games without an override.
- config/games/<path hash>.controls.cfg: game-specific controls.
- config/games/<path hash>.display.cfg: game-specific logical resolution.
- logs/runtime.log: native/Java diagnostics.

Profiles use the full game source path, like the existing RMS directory key.
Moving or renaming a JAR creates a different profile key. The package includes
no replacement profiles, commercial games, or user saves.

## Validation / build

01.14: native PPU build, host guarded-stack regression and ASan/UBSan MP3
lifecycle/budget tests passed. Actual console validation remains outstanding.
Historical build/test logs are retained and are not tests of this candidate.

Relink with `python3 tools/build_application.py --reuse-prebuilt-archives`
after obtaining the pinned PS3 SDK. Package with tools/package_application.py
--output PATH; add --crypto-libs PATH if the SDK signer needs libcrypto1.1.
The canonical native ELF is build/ps3/release_native.elf.

## 01.15 loading candidate

Read-only JAR/ZIP snapshots (8 MiB per file, 12 MiB total) reduce repeated
filesystem seeks and reads. Files outside that budget use the normal path.
RGB565 scaling now avoids divisions per TV pixel, with pixel-identical output.
These are loading-path optimizations; Bounce Tales startup completion and actual
PS3 load-time improvements still need validation. See RELEASE_NOTES_01.15.md
and build/validation/loading_runtime_0115.json for measured scope.
