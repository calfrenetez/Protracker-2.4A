# Additional sample slots, dev27

Sampler +SMP / Control-Shift-Plus appends an empty slot, up to 255, through shared
undo. One bounded metadata table retains existing PCM/markers without copying.
Undo refuses unexpected sample changes or remaining references and clamps the
selected sample. Source document storage and immutable sample versions retain
separate ownership; release works after successful document replacement.

All 34 host groups pass (43.238 seconds), including allocation/budget/revision/
binding refusal, zero/31/255-slot boundaries, 128-command eviction, exact stereo24
and marker import, slot/sample/note undo, earlier-slot metadata ownership, strict
MOD refusal, exact PTG identity, controller input and complete release. Cached and
full planar displays agree; the original main-screen golden is unchanged. The
native editor and PTSlotsTest cross-build without warnings. The build manifest
also now records the previously omitted song.h structural-owner header.

Final native run slots1789908937542494000 passed in 112.543 seconds. PTSlotsTest
executed all 0..255 slot cases on the emulated 68030. The editor added slot 32,
imported the first instrument from a separate MOD source, auditioned it through
Paula at period 428/volume 24, undid/redid slot/import/note edits and saved. Strict
MOD export refused the extra slot. Comparing all PTG chunks against a separately
converted baseline permits only the slot count, appended exact duplicate sample
record and intended instrument reference. CRC and byte-identical reopen/resave
passed. Both editor exits were clean; every DMA channel stopped. Fresh process,
private-HDF and socket checks verified emulator release.

The first functional run also passed, but one image preview appeared black. The
capture helper was strengthened to await a fresh complete PNG before proceeding,
and the full workflow was repeated. The final pre/post PNGs have identical hashes;
RGB stream inspection confirms both contain nonblack tracker/waveform pixels, and
the reopened display was visually verified. See screenshot-integrity.json. The
control guide correctly distinguishes the decimal SAMPLE counter (0032) from the
hexadecimal pattern instrument field (20).

Independent replay preparation includes a design grounded in the pinned CIA
source and sixteen synthetic flow MODs. Reproducibility, event offsets, strict
MOD preflight and no-overwrite checks pass on the host. They remain input fixtures:
no native flow traces or new sequencer implementation are claimed by dev27.

This is software/emulator evidence. No real AmiGUS, A1200 or ACA1234 test was
performed. General populated-slot deletion/reordering, high-resolution audition
and full enhanced song replay remain separate work.
