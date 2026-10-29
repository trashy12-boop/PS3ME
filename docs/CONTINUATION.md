# Current continuation: 01.04 controls

User confirmed 01.03 audio works on the real PS3 and Wonder Zoo plays. New request: remappable controls and right-stick pointer to simulate touch. Implemented native settings via browser Square, global recoverable config save, key remapping, right-stick arrow, R2 default press/hold/drag, speed/deadzone/touch-enable options. Start remains return. See README_PS3.md and docs/CONTROLS_0104.md.

Core host/sanitizer tests and native RPCS3 tests verified settings edits/save, Java key 53, pen press/drag/release, touchscreen capability flags, return to menu. Physical pad behavior and television cursor visibility remain pending. Do not infer physical validation from automated input. Audio regression uses 01.03 AudioSmoke. Real 01.04 package validation evidence is build/validation/controls_0104. Earlier test folders contain historical failures.

Rebuild the included TiMidity sources via tools/build_timidity.py before native linking; tools/build_application.py --reuse-properties retains the audio class library/native table exports. Added native Makefile header dependencies. Source bundle includes complete adapted VM/audio/control sources and sound bank/license, no user games or development trigger files.
