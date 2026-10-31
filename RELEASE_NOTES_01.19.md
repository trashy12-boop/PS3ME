# PS3ME v1.19 — Changes since v1.16

- Updated the game browser with Salsa64-inspired smooth vertical navigation and DualShock button prompts.
- Added **Português (Brasil) / English** language selection in settings.
- Added a localized **“Carregando jogo...” / “Loading game...”** screen.
- Updated the XMB app icon and refined button icon alignment, including X, SELECT and START.
- Reduced repeated SDL icon scaling, text rendering and unnecessary redraws in the menu (v1.18–v1.19).
- Organized the repository documentation: the main README is the entry point, with older version-specific READMEs retained in `docs/archive/`.

## Known issues (confirmed on real PS3 hardware)

- **The main menu still freezes after a few seconds.** Reproduced on v1.17, v1.18 and v1.19. **Not fixed.**
- **Pressing the PS button while in the menu can crash or freeze the PS3.** **Not fixed.**
- The new menu feels less smooth than Salsa64 on real hardware.

Earlier RPCS3 headless tests reached the menu, but they **do not** establish real-hardware stability. A structural PKG validation is not a crash fix.

**Status:** experimental build; use v1.16 if menu stability is essential. Back up `/dev_hdd0/game/PS3ME0001/USRDIR/appdb/` before upgrading.
