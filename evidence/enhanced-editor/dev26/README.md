# Position arrangement, dev26

POS ED. > MORE adds insertion, removal and movement of positions. Patterns,
sample data and numeric Bxx effects are preserved. The last position cannot be
removed. Before/after order lists join the existing shared journal and storage
owner; failures do not partially shift arrays. No-op equal moves preserve redo.
The editor now opens the pattern actually assigned to the first song position.

All 32 host groups pass (34.462 seconds), including sanitizer allocation/undo,
front/middle/end arrangement, outside-change conflicts, invalid indices, last
position protection, duplicate-position no-op, capacity and complete release.
The 256-pattern/position maximum still stays below the tested 6 MiB song-owner
bound. Full/cached display tests now include both position-control pages. The
main-screen golden remains unchanged. The first native build found one signed/
unsigned conditional warning; an explicit unsigned literal fixed it. The final
68000/soft-float cross-build passes with warnings as errors.

Native run arrange1789906972324012000 passed in 131.943 seconds. PTSongTest and
PTPaulaTest passed. The editor built a second pattern, entered a note, inserted,
removed and moved positions, undid/redid the sequence, and exported the expected
MOD bytes with orders [1,0,1]. Pattern/sample bytes outside the intended new note
are identical to the fixture. Playback started on order 2/pattern 1, period 428.
A reorder during playback, with unchanged order count, stopped the stale snapshot;
undo restored the saved state and restart used the restored sequence. PTG CRC and
byte-identical reopen/resave passed. The reopened editor displays pattern 1 at
position 0. Both editor exits were clean, all four DMA channels stopped, and
process/HDF/socket checks confirmed emulator release. Screenshots were inspected.

This is software/emulator evidence. No AmiGUS, A1200 or ACA1234 physical test was
performed. Pattern deletion and existing-song channel/sample-slot resizing remain
separate work.
