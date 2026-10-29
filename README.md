# PS3ME: J2ME on PlayStation 3

**Experimental native PS3 port of [PS2ME](https://github.com/Wellinator/PS2ME)** (the original PlayStation 2 J2ME emulator). PS3ME uses a PS3-native runtime, not PS2 backwards compatibility.

## Tested on actual PS3 hardware

**The project owner has tested PS3ME on a real PS3, and reports that almost every game they tried works.** Individual games still have compatibility issues, and crashes, missing sounds, or freezing are possible. This is **experimental software**, not a claim of universal compatibility. The specific **01.14 MP3 stack-overflow fix** passed host and build tests but had not yet been verified on physical PS3 hardware when packaged; earlier real-console tests do not prove that fix works.

## Download / install

Get the downloadable `.pkg` from [GitHub Releases](https://github.com/trashy12-boop/PS3ME-a-PS2ME-Fork/releases). Enable PS3 homebrew support (e.g. HEN) and install the PKG. When upgrading, install over the existing installation so existing game data remains intact. Title ID: `PS3ME0001`.

**Where to put games:** Place your own legally obtained J2ME `.jar` or `.jad` games inside:
```text
/dev_hdd0/game/PS3ME0001/USRDIR/games/
```
You can also browse a USB drive at `/dev_usb000` or `/dev_usb001`, including subfolders. The launcher opens its internal games directory by default. Games are not included with the emulator.

## Controls

### Game browser / library

| Button | Action |
|---|---|
| **D-pad Up / Down** | Move through the list |
| **X (Cross)** | Open selected folder or launch selected JAR/JAD |
| **O (Circle)** | Go up one directory |
| **L1** | Jump to first USB drive (`/dev_usb000`) |
| **R1** | Jump to internal storage (`/dev_hdd0`) |
| **Select** | Refresh game list |
| **Square** | Configure the global default game controls |

### Inside a game: default phone-key mappings

| PS3 button | Default Java ME action |
|---|---|
| **D-pad Up / Down / Left / Right** | Phone navigation arrows |
| **X (Cross)** | Fire / center / OK |
| **O (Circle)** | Unassigned by default |
| **Square** | Phone `*` |
| **Triangle** | Phone `0` |
| **L1** | Left softkey (typically Menu) |
| **R1** | Right softkey (typically Back) |
| **L2** | Phone `#` |
| **R2** | Touch / hold |
| **Select** | Unassigned by default |
| **L3 / R3 individually** | Unassigned by default |
| **Left analog stick** | Alternate navigation arrows |
| **Right analog stick** | Move virtual touchscreen pointer (when enabled) |
| **Start** | Exit running game and return to the game browser |
| **L3 + R3 together** | Open the in-game settings menu |

Some games interpret softkeys differently. You can remap D-pad, face buttons, shoulder buttons, Select, L3 and R3 to phone arrows, Fire, softkeys, numeric keys 0–9, `*`, `#`, touch/hold or disabled.

### In-game settings (L3 + R3)

Up/Down selects an option. Left/Right or X adjusts values. Configure resolution (176×220, 240×320, 320×240, 320×480, 352×416, 480×800), game-specific button mappings, virtual cursor enable/speed/deadzone, then save settings for this game, restore global defaults or resume. **Circle or Start resumes the game while this settings menu is open.** Choosing **Return to game library** exits the running Java session.

For landscape games choose 320×240, not 240×320. Some JARs require their own landscape edition.

## Saves and configuration

Base folder: `/dev_hdd0/game/PS3ME0001/USRDIR/`

- `games/`: JAR/JAD games.
- `appdb/`: installed Java suites and RMS saved data.
- `config/controls.cfg`: global button configuration.
- `config/games/`: per-game button and resolution settings.
- `logs/runtime.log`: diagnostics.

Moving/renaming a JAR can make its saved control profile appear different because the game path is part of its key.

## Credits and AI development disclosure

- **[Wellinator / PS2ME](https://github.com/Wellinator/PS2ME):** Original PS2 emulator and upstream project. PS3ME is an unofficial independent port.
- **[salsa64](https://github.com/salsa64):** UI styling inspiration. No endorsement or authorship of this PS3 port is implied.
- **OpenAI Codex / ChatGPT:** Substantial AI generation and revision of the port code, scripts, tests and documentation.
- **Project owner (trashy12-boop):** Project direction, feature requests, feedback, and physical PS3 hardware testing. **The owner does not know how to code and openly describes this as a “vibe-coded” project.** The project does not falsely claim they personally hand-coded it.
- **phoneME, SDL_mixer, TinySoundFont/TinyMidiLoader, TiMidity++, TimGM6mb, DejaVu and other upstream contributors:** Retain their original copyright notices and licenses. See [third-party notices](docs/THIRD_PARTY_NOTICES.md).

This is not affiliated with or endorsed by Sony, Wellinator, salsa64 or OpenAI.

## Licenses and source

**Project GPL-2.0-only**, inherited from PS2ME; see [LICENSE](LICENSE). Third-party dependencies and assets have their own licenses and exceptions, described in [THIRD_PARTY_NOTICES](docs/THIRD_PARTY_NOTICES.md). Preserve notices when redistributing. See also [AI disclosure](docs/AI_DISCLOSURE.md).

This GitHub repository may still be importing the complete 01.14 source tree from the attached source ZIP. Refer to the release asset for the full snapshot until the import finishes. PS3 toolchain/firmware and commercial games are not bundled.

## Versions and tags

Tags identify a **specific code snapshot**. Use `v01.14` for the 01.14 build; use **“PS3ME 01.14 | Experimental Alpha”** as the release title. The *pre-release* checkbox indicates unstable/experimental status. **“Alpha” belongs in the title/status, not the version tag.** Next release: `v01.15`, then `v01.16`. This makes downloads and updates unambiguous.

See [01.14 release notes](RELEASE_NOTES_01.14.md).
