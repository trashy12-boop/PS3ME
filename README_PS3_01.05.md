# PS2ME / PS3 01.05

Install PS2ME_PS3_01.05.pkg over 01.04 with HEN enabled. Title ID is PS3ME0001.
The package contains no commercial games and does not replace saves or controls.

Changes:
- Independent MIDI players for music/effects, replacing the global music stream.
- TimGM6mb General MIDI SoundFont, native endian float PCM at the audio device
  rate (48 kHz on PS3), bounded voices and output clipping/NaN protection.
- MIDP suite/storage state is finalized when returning to the browser, allowing
  the next selected game to use its own installation and RMS directory.
- Salsa64-inspired vertical browser, blue/purple waves, larger selected title
  and embedded JAR PNG icons (MIDlet-Icon, otherwise MIDlet-1's icon field).
- Select rescans the folder and clears the browser's in-memory icon cache.
  A persistent disk game-list cache is not included in this reconstruction.
  JAD files still launch, but their icons fall back to the default icon.

Browser controls: D-pad selects; X opens; Circle parent; Square controls;
Select refresh; Triangle game resolution; L1 USB; R1 HDD; Start exit.
In game: Start returns to browser. Right stick moves the touch arrow; R2 presses
and drags. Existing saved remapping and touchscreen settings remain supported.

Base directory: /dev_hdd0/game/PS3ME0001/USRDIR
- games: place your JAR/JAD files here, or browse HDD/USB.
- appdb: per-source-path installed suites and RMS saves. Keep this directory.
- config/controls.cfg: mappings and touch settings.
- soundfont: offline instruments, license and provenance.
- ui: font and license.
- logs/runtime.log: native and Java diagnostics.
Old timidity files from 01.04 are unused and may remain installed. Moving or
renaming a game changes its save path. Refresh does not reinstall a previously
installed suite or delete RMS.

This is a native PowerPC64 PS3 application, not a PS2 Classics wrapper. New audio
and switching behavior need confirmation on physical hardware. Automated test
results are retained under build/validation/release_0105 in the source archive.
The earlier user reports confirm 01.04 gameplay, audio and touch only.

MMAPI limits: stream MIDI and SDL_mixer-supported WAV; no MP3/AMR claim, no
nonzero seeking, and completion events still depend on Java state polling.
16 player slots and 64 voices per MIDI player; unusual files can exhaust voices.

Rebuild: fetch the pinned SDK/phoneME via tools/fetch_deps.py; copy vendor/phoneME
onto deps/phoneME; run python3 tools/build_application.py --reuse-properties.
The archive includes preverified classes and genuine VM/MIDP/PCSL archives.
The application now uses SDK SDL_mixer and MidiEngine.cpp; the old TiMidity
rebuild is unnecessary. Output: build/ps3/PS2ME_PS3_01.05.elf.
Package via tools/package_application.py --output PATH; its signing tool may
need --crypto-libs pointing to libcrypto1.1. Package validation compares every
file with staging. SDK binaries, firmware and commercial game assets are excluded.

Menu reference: https://github.com/ricowp81/Salsa64 at
4a94c2d35b71bb0e51e7b73fd0f3405d326056ce. Independent SDL implementation.
TinySoundFont/TinyMidiLoader: MIT, third_party/tinysoundfont/LICENSE and
PS3_CHANGES.json. TimGM6mb: GPL-2.0, assets/audio/soundfont/COPYRIGHT.
DejaVu Sans: assets/ui/COPYRIGHT. Existing project/phoneME/SDL notices retained.
