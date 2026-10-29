# JAR-launch freeze investigation — build 01.02

The recovered 01.01 menu was shown on the user's PS3, but opening the diagnostic froze it. We reproduced a native VM failure in RPCS3 in the preceding debugging session; those unsaved working files were lost during environment cleanup. This continuation recovered source bundle version 7 and reconstructed the recorded changes from real source.

## Implemented fixes

1. ExecutionStackDesc::relocate_internal_pointers used jint* for a saved frame pointer. On big-endian PPU, the first 32 bits of an ordinary low-address 64-bit pointer are zero. The relocation was skipped; restored frames then pointed into stale stack memory. Use address* and preserve pointer width in stack growth.
2. MIDP's native allocator had 32 MiB, while the Java heap retained its 1 MiB default. Configure the actual JVM heap separately (32 MiB capacity, 8 MiB minimum).
3. Installer.getUrlPath removed the slash from every absolute file URL. Keep POSIX absolute paths; strip only the slash before a Windows drive letter.
4. jarCRC32 used unsigned long and returned its complement. On LP64 the complement included a high all-ones word, unlike the ZIP's 32-bit CRC. Use uint32_t for the accumulator.
5. ConstantPool::wide_bits_at confused memory address ordering with high/low significance. jlong_from_low_high means the two native memory words, despite its name. Convert using jlong_from_msw_lsw with the target word order. A real native trace recorded a requested installer sleep of 4294967295895 ms; after correction the native delay was 872 ms. Do not "fix" KNI by swapping its already correct memory-word order.
6. Generic VM OsFile_path_separator_char was semicolon while MIDP generated colon-separated classpaths. The second VM startup treated game.jar:classes.zip as one directory, could not load java/lang/Object and reported fatal error 133. Use colon for the PS3 generic OS port.

Temporary method-by-method and native sleep tracing was removed before the release build. The permanent diagnostics report initialization, install status, actual MIDlet execution and framebuffer milestones. All source patchers reapply successfully to the saved adapted snapshot.

## Current execution evidence

The final 01.02 installed EBOOT has passed the static PPU interpreter test in RPCS3: fresh installation, actual suite launch, 26 ms Java sleep, allocation/GC, RMS counter 1, Canvas paint 240x302, RGB565 bands received by the 240x320 native framebuffer, automated Start handler, MIDP return -1 and 120 browser frames. The -1 is the result of explicitly stopping the VM, not a claim that every MIDlet ended normally.

A prior development run reopened the diagnostic and read counter 2 from disk in another emulator process. The final signed-package LLVM run also passed every diagnostic and return milestone, loaded RMS counter 2, and observed 120 menu frames. Its 60-second harness result is in release_0102/pkg_llvm_warm. The earlier 45-second LLVM run timed out while compiling emulator modules, before application startup. Use the structured result files as the authoritative outcome, not old failed logs.

The headless Null renderer does not prove visible graphics. The automated Start request calls the same native handler but does not test a physical controller. The PS3 user must still validate this new build. Java ME audio and commercial game compatibility are not established.

## Rebuild evidence

The actual PPU compiler compiled ExecutionStackDesc, Thread, Interpreter_c, Natives, Universe, ConstantPool, kni, the MIDP jar.c reader, and the modified native platform/entry files. The affected objects replaced the real static-library members, then the application was linked, stripped, post-linked and signed. The Installer and launcher were compiled with JDK 8 and passed the actual legacy CLDC verifier via qemu-i386-static. Both diagnostic classes were explicitly preverified.

The finalized PKG is independently decrypted and every payload is compared exactly with staging. The standard PSL1GHT NPDRM zero padding is materialized in the staged EBOOT before packaging; do not mistake the extra 48 zero bytes for stale data or delete them from the installed file. All 162 files from a fresh RPCS3 installation match staging byte for byte.

The emulator's --installpkg/--installfw processes abort during teardown in this environment after successful installation; this is recorded separately from application runtime. Firmware is not included in the deliverables.

The source snapshot and exported libraries must remain together: relinking a library export alone does not compile a new VM source edit. The library archive and final ELF hashes in the validation results identify the actual tested state.
