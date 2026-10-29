# PS2ME for PS3 01.08

This release packages the native PS3 build with the supplied 320×176 XMB icon. The release PKG contains no internal diagnostic/test JARs; the packaging script now excludes them by default. The source repository retains the development fixtures and tests.

The release package passed structural validation as a PowerPC64 big-endian PS3 homebrew PKG. Version 01.08 has not been tested on a physical PS3 or in RPCS3. The user previously reported successful JAR gameplay, touch input, and audio with the 01.05 build; 01.06 menu/profile behavior was tested in RPCS3.

The port was developed with substantial AI assistance from OpenAI Codex at the user's direction. The original PS2ME source snapshot is credited to Wellinator and is identified in `upstream/PROVENANCE.json`; see `docs/AI_DISCLOSURE.md` and `docs/THIRD_PARTY_NOTICES.md`.
