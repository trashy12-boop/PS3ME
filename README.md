# PS3ME

**An experimental PS3 port of [PS2ME](https://github.com/Wellinator/PS2ME).** PS2ME was originally developed for PlayStation 2. PS3ME runs natively on PS3 rather than through PS2 backwards compatibility.

**⚠️ Experimental software: Stability and compatibility are NOT guaranteed for any J2ME game, including games that worked in previous tests.** Individual games may crash, freeze, run slowly, behave incorrectly, or have broken sound/controls. Back up any important saved data before use.

## Tested on actual PS3 hardware

**Trashy12 has tested earlier PS3ME builds on a real PS3 and reports that most tested games work.** 01.16 has host and RPCS3 validation, including Bounce Tales reaching its menu and interactive Chapter 1 gameplay. **01.16 still needs physical PS3 verification.** Earlier hardware tests do not guarantee compatibility or stability for this update or every game.

See the [01.16 compatibility audit](docs/COMPATIBILITY_AUDIT_01.16.md) for restored APIs, source comparisons, test evidence and remaining unsupported features.

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
- **Project owner (Trashy12):** Project direction, feature requests, feedback, and physical PS3 hardware testing. **The owner does not know how to code and openly describes this as a “vibe-coded” project.** The project does not falsely claim they personally hand-coded it.
- **phoneME, SDL_mixer, TinySoundFont/TinyMidiLoader, TiMidity++, TimGM6mb, DejaVu and other upstream contributors:** Retain their original copyright notices and licenses. See [third-party notices](docs/THIRD_PARTY_NOTICES.md).

This is not affiliated with or endorsed by Sony, Wellinator, salsa64 or OpenAI.

## Licenses and source

**Project GPL-2.0-only**, inherited from PS2ME; see [LICENSE](LICENSE). Third-party dependencies and assets have their own licenses and exceptions, described in [THIRD_PARTY_NOTICES](docs/THIRD_PARTY_NOTICES.md). Preserve notices when redistributing. See also [AI disclosure](docs/AI_DISCLOSURE.md).

Source, build scripts and dependency pins are included here. Release source ZIPs also retain the recovered native archives/ABI headers and validation logs used for incremental builds. PS3 SDK/firmware and game JARs are not bundled.

## Versions and tags

Tags identify a **specific code snapshot**. Versions use `v1.14`, `v1.15`, `v1.16`, and so on. Experimental/Alpha belongs in the release title and pre-release status. Current development build: **01.16**. See [01.16 release notes](RELEASE_NOTES_01.16.md).
