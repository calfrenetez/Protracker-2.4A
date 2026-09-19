# Border alignment and playback presentation, dev11

The owner noticed misaligned lines. The left 19-pixel rows and right 18-pixel
rows now share a 19-pixel grid, including scope/version boundaries. Pattern rails
continue through a single header to the bottom bar. The sample/status strips have
distinct edges, and long numeric values no longer overwrite a vertical bevel.

A related native presentation bug retained old playback rectangles. Native ticks
and cached updates now share one rectangle list, and the BPM clear width covers
the final glyph column. Host tests check fixed border pixels, mouse row boundaries,
full/incremental equivalence and tick-only presentation while tempo digits change.

`core-build.json` identifies the final binary and sources. `host-tests.log` retains
23 passing groups. `editor/` contains native edit/save/undo/reopen checks, the actual
layout screenshot and an optional playback check. That check adds MOD effect F96
at row8, waits for 150 BPM during live replay, and compares the screen's tempo field
with an independently generated font bitmap without forcing a full redraw. It
then verifies all four audio DMA channels are off and the editor exits normally.
The native/host stopped-screen pixel comparison is recorded separately.

One emulator window was explicitly released by AmiConnect, used only for the
private exact profile, shut down with the guarded QUIT, and released after fresh
process/private-disk/socket checks. Physical hardware acceptance remains NOT RUN.
The screenshot is actual native output; enlarged images use nearest-neighbour
scaling. Visual acceptance by the owner remains pending.
