# Reference refinement, dev9

The owner said the main screen was nearly there and supplied the reference again.
This increment refines the actual 640x512 planar renderer: reference proportions,
field-specific glyph spacing, darker grey panels, brighter blue notes, raised
edges, channel dividers, deeper scopes, taller bank/transport controls and a
classic ready-status strip. The existing project fixture remains editable data;
its stopped scope lines are not fabricated playback. Route letters and 2.4G
branding remain. This is not a claim of owner visual acceptance.

`core-build.json` records final source and binary hashes. `host-tests.log` records
23 passing groups, including address/undefined-behaviour checked editor tests.
The editor tests cover every relocated pattern digit cell, bottom controls and
full versus incremental pixels, including selection, playback and long messages.

`editor/` retains the final build's native edit, undo/redo, exact project save,
existing-file refusal, reopening, reference capture and normal-exit results.
`mouse/` retains the same build's input.device command/button checks, playback,
audition, DMA shutdown and normal exit. Each report records its binary hash.
`capture-comparison.json` records pixel comparison between the native capture and
host rendering of the same project: the only differences are in the 22x22 mouse
pointer area. The enlarged output is an exact 2x nearest-neighbour crop of the
actual native capture; it adds no artwork or effects.

Amiberry ownership was explicitly released by AmiConnect for these tests. Exact
profile-guarded shutdown and absent process/disk holder/socket were checked before
release. Physical AmiGUS and ACA1234 acceptance remain NOT RUN. Earlier feature
validation retains its own build identities under dev3 through dev8.
