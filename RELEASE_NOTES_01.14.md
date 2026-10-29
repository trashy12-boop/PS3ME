# PS3ME 01.14 | Experimental Alpha

**IMPORTANT: PS3ME is experimental. Stability and compatibility are not guaranteed for any J2ME game, even one previously tested successfully.** Crashes, freezes, audio or control issues, regressions between versions, and loss of unsaved progress are possible. Keep backups of important saved data.

## Real PS3 testing
The project owner has tested PS3ME on physical PS3 hardware and reports that almost all of the games they tried work. This does not imply full Java ME compatibility; specific games can fail, freeze or have audio and input issues.

**Version-specific distinction:** The 01.14 MP3 decoder change was validated by compiler/host-side checks, but 01.14 had not been independently verified on the console when packaged. Hardware successes reported for earlier versions should not be misrepresented as verification of every 01.14 fix.

## What's changed in 01.14
In 01.13 the MP3 decoder could exceed the PS3 SDL audio callback thread's 16 KiB stack. The 01.14 code moves MP3 preparation to a joinable 256 KiB worker, then plays back bounded PCM without decoding on the callback. It enforces a shared 16 MiB native audio budget and keeps MIDI/WAV and Player replay fixes.

Tests: guarded host stack regression, PPU compiler stack audit, ASan/UBSan checks of 45 MP3 assets and package structure checks. Host validation cannot guarantee console results.

## Installation and games
Install `PS2ME_PS3_01.14.pkg` with PS3 homebrew enabled. Preserve the existing installation and game saves when updating. Copy legally obtained `.jar` files to `/dev_hdd0/game/PS3ME0001/USRDIR/games/`, or browse games from USB.

## About
**An experimental PS3 port of [PS2ME](https://github.com/Wellinator/PS2ME).** Its UI is inspired by salsa64's work. This project is openly vibe-coded: substantial code was produced through AI assistance under the guidance of its owner, who does not claim coding expertise and tested it on real PS3 hardware. GPL-2.0-only for the covered project code, with separate third-party component licenses and notices. See README and LICENSE.
