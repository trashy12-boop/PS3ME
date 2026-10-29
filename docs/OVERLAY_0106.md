# 01.06 validation

The installed EBOOT.BIN was exercised in RPCS3 0.0.43-20412, PPU static
interpreter / Null renderer. The 171 installed package files matched staging.
Our automated fixture opens the real native settings menu after the first Java
frame, changes resolution and a mapping, saves, and resumes. The first run
observed Java sizeChanged from 240x320 to 320x240. A second process using the
installed EBOOT started at the persisted 320x240, then changed to 320x480 and
returned to the browser (120 frames). Host ASAN/UBSAN verifies game A settings
are not applied to B, and tests fallback/reset/invalid dimensions. Native menu
surface was visually inspected. Chord opening with a physical DualShock and
commercial-game live resizing still require console validation.

The test harness Diagnostic-specific booleans are false because this test uses
TouchSmoke in the emulator-only diagnostic slot, not the original Diagnostic
MIDlet. Forced Start exits return -1. Those are not launch failures. Automated
trigger files, emulator profile files and replacement fixture are not packaged.
The source contains the test and actual retained logs.
