# Queued private EFx core qualification

2026-09-26 UTC. Incremental private sample-bank producer only; sampler/editor
Studio entry points still refuse EFx. No native PLAY, transport, audio quality,
real-time performance or physical acceptance is implied.

Host: 23 renderer/Studio/bounce unittest cases pass with sanitizer fixtures
(98.991s). New queue fixture compares every emitted PCM value with the offline
EFx renderer across1/17/256-frame partitions, normal/lead-in/row-range playback,
transport backpressure and delayed completion. Producer stop/close preserves a
consumer-held queue copy even when cancellation fails; only confirmed release
permits queue destruction. Every open allocation failure and sample-budget
refusal frees owned storage; source masters remain unchanged.

Native: canonical pinned m68000/nix20 compiler with -fbbb=-; exact compiler,
runtime, source/header and binary hashes in native-build.json. Shared030 run
render-files-1790465268200545000 passes RC0 using256-frame normal/lead-in/range
checks and a17-frame held-lease stop case. The production allocator reports53
Fast/not-Chip allocations, zero owned bytes, and budget refusal without Chip
fallback. Run completed within120-second deadline. All four audio DMA channels
were off; exact run/launch paths removed and verified absent. Window explicitly
released to AmiConnect and Scott; no lifecycle/config/physical operations.

The offline renderer and queue producer share audited tick/voice operations;
this parity check validates incremental scheduling/ownership, not an independent
ProTracker effect oracle. Existing pinned-reference EFx evidence remains in the
invert-ordering/invert-oneshot directories. Existing23 host regressions cover
ordinary rendering, command sequences, direct true24 Studio mixing and bounce.
