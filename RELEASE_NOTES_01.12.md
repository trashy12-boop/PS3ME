# PS2ME PS3 01.12 — stability candidate

- Serialize native Player lifecycle/commands to prevent concurrent slot mutation.
- Free native encoded input/RWops once WAV or MIDI decoding completes.
- Load the shared MIDI sound bank only when MIDI is requested.
- Remove the 01.11 soft limiter from the real-time callback.
- Preflight WAV expansion and enforce a 16 MiB aggregate decoded-WAV budget.
- Detach mixer callbacks before tearing down audio resources.

The 32-player cache and WAV retrigger behavior are retained. Release PKG omits
test/commercial JARs. Host ASan/UBSan stress tests passed; physical PS3 and RPCS3
validation of 01.12 are pending. These changes address code-level risks, not a
reproduced console crash; no single crash cause has been proven.
