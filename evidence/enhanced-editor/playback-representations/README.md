# Derived PCM and high-precision master preview — 2026-09-23

103 host tests PASS172.415s, including ASan/UBSan packed representation tests.
Complete pinned native cross-build passed. Shared 68030 emulator ran the cache,
playback-PCM and Paula regressions; all three returned0 in result.json.
Tests cover true24 source preservation, signed rounding/clipping, explicit
8/16-bit output, byte order, stereo channel selection, word padding, format-key
coexistence and revision invalidation. Paula preview showed period428 and a
six-byte Chip copy of a five-frame 24-bit master, preserving master bytes/bits/
frame count. Existing cache reuse, pressure, scope and replay checks passed.

The first native test used a fixed five-tick delay, earlier than the speed6 first
row trigger, and failed its new observation assertion. It exited cleanly with
DMA off. The final test waits for the first nonzero replay period (observed at
tick7 in this run); no product-code change was required. Both results are retained.
CIAA fallback execution remained skipped because Workbench owns that vector.

Limits: Paula preview supports mono at the classic rate and existing classic
length/loop restrictions. Full enhanced-song dispatch, stereo/rate conversion,
AmiGUS allocation/upload and direct Studio streaming remain open. No physical or
analogue acceptance. All DMA channels off, run files cleaned, lock/resource
released; shared guest left running. Unrelated uncommitted display work is not
linked into these tests; no clean full-editor release package is claimed.
