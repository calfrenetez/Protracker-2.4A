# Allocator-backed render workspace

All105 host tests passed (187.066s), including allocated and legacy core/file/stem
regressions. Allocator tests fill workspace with nonzero bytes, reject allocation,
check report preservation and enforce one live allocation with full release on
success, preflight refusal, cancellation and sink failure. PCM checks retain
true24 source values. Existing short-read/EINTR and staging failure checks pass.

Native stack.su reports per-function compiler estimates, not total stack peaks
or physical performance. Allocated entry frames are64/72 bytes, legacy2364/5532;
shared helper frames336/488. File buffers and runtime resources remain outside
this workspace. Bounce internal stream and live Studio output remain open.

The full host suite preceded the final native-only bounce preflight error mapping
(MEMORY to CAPACITY); the subsequent pinned native rebuild covers that line.
Unrelated dirty display changes are preserved. No UI/release candidate or physical
A1200/AmiGUS acceptance follows from the file/core regressions.

Final pinned native rebuild passed. Shared030 all five binaries eventually
returned0 with expected markers, but the combined suite exceeded its90-second
observer deadline. Original result.json retains that failure; later-completion.json
records the subsequent done marker and rc0 checks. No relaunch/reset occurred.
Reacquired shared lock and target guards before exact owned run/launcher cleanup;
AmiConnect explicitly released. This is functional execution evidence, not a
90-second performance pass. Runner now selects legacy(default) or allocated-only
sets to avoid combining all five; this selection change was syntax-checked but
not rerun. No physical hardware operation.
