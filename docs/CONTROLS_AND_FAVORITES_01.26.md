# Favorites and LG C397 controller editor (01.26 draft)

## Game favorites

- Press **R2** in the game browser to add or remove the selected JAR/JAD from
  favorites. A `[*]` marker identifies games already saved as favorites.
- Press **L2** to switch between the normal browser and the **Favorites** tab.
  Press Circle from Favorites to return to the normal browser.
- Favorites are kept in `USRDIR/config/favorites.cfg`; updates use a temporary
  file and recovery copy. The list can include HDD and USB games. If a USB drive
  is disconnected, its favorite stays listed and can be removed with R2.
- Opening a favorite launches the same path as the normal browser, so the
  existing per-game settings and save-data key remain unchanged.

## Visual controller mapping

The global controller settings and the in-game settings use the physical
keypad crop from the LG C397 reference image supplied for this project. The
currently assigned J2ME action is drawn as a translucent blue highlight over
the matching D-pad, softkey, or keypad key. The mapping list remains visible
beside the image.

- Up/Down selects a PS3 input; Left/Right or Cross changes its J2ME action.
- Circle saves the global mapping; Start cancels. Triangle restores defaults.
- Per-game mappings continue to use the in-game menu's existing **Save for this
  game** and **Use global defaults** actions.

The source image crop is `assets/ui/lg_c397_keypad.png`; the selection tint is
rendered at runtime, not baked into the crop.

## Validation

`make -C ps3 test-favorites` checks persistence, removal, HDD/USB path handling,
path traversal rejection, and recovery from a damaged primary config. The PPU
application built and the PKG passed structural/payload validation. The editor
and browser still need real-PS3 input/rendering validation before a release.
