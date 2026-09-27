# Native editor master-preserving workflow,27September2026

Committed1ac4856 editor candidate251548bytes,
SHA256 2fb5fd31a7fe33731adf159a2e538686dcae833edb0dbd6ab24b9c6e0d8ff50e,
passed bounded480second shared030 workflow invert-ui-1790548903883714000.
Fixture invert_swaponce.mod and committed host-reference WAV/stems were independently
validated by this turn's host tests. Native UI checks cover requester cancellation,
exact24-bit WAV and two stems, bounce into a new slot, original master records
unchanged, exact bounced PCM, verified save, undo/redo and clean exit. Screenshot
was inspected and retains the classic layout. No unrelated display edits used.

The recents prefix was backed up, isolated and checked before launch; previous
presence and bytes were restored before cleanup. All4DMAoff, exact run/launcher
cleanup and independent absence verified; AmiConnect window explicitly released.
The runner now uses shared running-state checks and observed cleanup, and refuses
successful cleanup if settings restoration cannot be established. Cleanup tolerates
an already-absent owned launcher while still requiring observed final absence.
Three host guard groups pass including recreated directories/links and missing
launcher, failures, incomplete runs and preservation of unrelated files.

No actual device playback, AmiGUS, physical Amiga, live Studio or timing acceptance.
This is native editor file-workflow evidence; it does not implement native PLAY.
