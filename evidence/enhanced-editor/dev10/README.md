# Button typography and relief, dev10

The owner asked to improve the fonts and pseudo-3D effects. This pass retains the
pinned bitmap font while correcting crowded label spacing and giving the text
consistent lower-right shadows. Buttons have two-step bevels and shaded faces;
selected controls reverse the bevel and shift their lettering inward. Arrow
controls receive the same raised treatment. The existing layout and mouse hit
regions are unchanged. No image artwork replaces the actual renderer.

`core-build.json` binds the final native binary to its source. `host-tests.log`
records all 23 passing host groups, including exact full/incremental pixel
comparisons with selected controls and secondary panels. `editor/` records this
same binary's native editing, undo/redo, exact saves, existing-file refusal,
reopening, actual reference-layout capture and normal exits. The native capture
is compared with the host renderer in `capture-comparison.json`; only the mouse
pointer differs. Enlarged output images are nearest-neighbour crops of that
native capture.

This pass does not claim a new native mouse regression or physical hardware
acceptance; prior mouse evidence is retained with its own binary under dev9.
AmiConnect explicitly released Amiberry for the single bounded native check.
Profile-guarded shutdown and process/private-disk/socket absence were verified
before release back to AmiConnect. Owner visual acceptance remains pending.
