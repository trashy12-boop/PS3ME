# PS3ME 01.23 — XMB display heartbeat

Replaces the 01.22 v8 timed probe and fake flip-completion mechanism.

The PS3 system overlay needs application flips to make progress. The idle menu
now repeats the retained framebuffer at up to 30 FPS, including while the XMB
is open. Game sleep and native settings loops also keep the display serviced.
Application input stays gated until a real system close/end event; a completed
flip never fabricates MENU_CLOSE.

Before an SDL upload, check the real completion of the previous flip. If that
flip is pending, defer the upload before touching the RSX framebuffer. Remove
the GPU flip wait: synchronization happens on the CPU admission check, leaving
the GPU stream available to the system compositor. No timeout fabricates a
completed flip, and there is at most one successfully submitted flip in flight.
Register one application callback on slot 1; SDL retains slot 0. Clear held
application buttons during the system overlay. Keep existing game APIs, assets,
audio, controls, saves, and per-game settings.

Validation: native PPU build and ELF checks; sanitized host tests for a 20-second
pending flip, a further delayed flip during XMB, retained-image heartbeat,
real event-driven resume, duplicate callbacks, and the three SDL presentation
entry points; existing controller regression; finalized PKG with every payload
compared against staging. No test JARs are included.

Physical PS3 verification is pending. Host models do not reproduce firmware or
HEN and are not proof that this resolves every console freeze. RPCS3 was not run
for this build.

Reference (behavior studied; no Salsa64 source copied):
https://github.com/ricowp81/Salsa64/tree/171aeb72fd526a5fb4c191eba77caa547c030241
See docs/hilos-ppe.md (Botón PS y pausa) and lib/rsxkit/src/video.c
(rsxkit_frame_end, CPU flip admission).
