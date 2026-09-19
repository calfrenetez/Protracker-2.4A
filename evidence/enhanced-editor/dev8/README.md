# Staged new-song and final editor evidence

Binary SHA-256: `f9283774a815253f359b102ef18f1ea0097aa28cf1a8be494705733c07a09783`.

Both current native runs use this executable. The document harness verifies
all 1..16 channel counts, round trips, every allocation-failure point and memory
budget rejection while preserving the prior song. The actual UI replaces a
playing classic MOD with a new 16-channel song, confirms DMA stops, saves a note
on channel 16, cancels a dirty replacement and retains undo/redo, then confirms
a new single-channel song and saves it through the native requester. Event/sample
fields, dimensions, routes and CRC are checked independently. Reopening and
resaving the 16-channel project is byte-identical.

The final keyboard/history/save/reopen regression also opens the retained layout
fixture and captures the actual native reference screen. The host suite passes
23 groups, including full-versus-incremental image and dirty-region comparisons.
Count and Create-confirmation changes redraw only their own control rectangles.

Earlier MOD-export/original-2.3F, block/mouse and replay proofs keep their original
build identities in dev4-dev7. Those historical runs are not relabelled as this
binary. Physical AmiGUS, mixed playback, sample import UI, existing-song resizing
and hardware/display acceptance remain open.
