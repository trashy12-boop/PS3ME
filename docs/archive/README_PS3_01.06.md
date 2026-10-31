# PS2ME / PS3 01.06 — in-game settings and per-game profiles

Install PS2ME_PS3_01.06.pkg over 01.05 with HEN enabled. Keep the existing
application and appdb saves. Title ID remains PS3ME0001. The 01.05 MIDI engine,
SoundFont and working touchscreen path are retained.

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

Compiled for big endian PowerPC64 / Cell LV2. Host sanitizer tests verify profile
isolation, resolution/mapping reload, fallback, reset and invalid dimensions.
RPCS3 tests exercise the actual native menu, save and Java sizeChanged callback.
Physical L3+R3 operation and compatibility with individual games still need PS3
hardware confirmation. Source includes tests and retained validation evidence.

Build with the pinned SDK/phoneME setup from earlier releases, then
python3 tools/build_application.py --reuse-properties. The canonical ELF for
packaging is build/ps3/release_native.elf. Package with tools/package_application.py
--output PATH; libcrypto1.1 may be needed for the SDK signing tool. Build exports,
preverified classes and source/licenses are included; SDK/emulator/firmware and
commercial game bytes are excluded. Browser and overlay styling are inspired
by https://github.com/ricowp81/Salsa64; these SDL menus are our implementation.
