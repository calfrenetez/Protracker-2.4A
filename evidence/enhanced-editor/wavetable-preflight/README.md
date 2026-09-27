# Whole-sequence wavetable preflight — 27 September 2026

The mono dispatcher and silent analysis now share their capability rules. The
preflight advances the complete audited renderer sequence before a caller can
choose to start output, including silent pre-roll and delayed rows. It reports
late unsupported stereo/segment/repeat operations and releases both allocator
workspaces on success, renderer refusal or allocation failure.

Final host ASan/UBSan fixture: PASS,5.851s. Native build:72 dependencies verified.
Shared030 run `render-files-1790477101421455000`: RC0 within90s;73 Fast allocations,
zero final owned bytes, budget refusal without Chip fallback. Separate fresh
AmiConnect/Scott coordination, shared lock/live identity guards, all4DMAoff,
exact run/launch cleanup and explicit release verified. No remaining hold.

Binary175576 bytes; SHA256
`6e97821ae1a3d6627da2d1b21cee40e3e6708d37e6ae89d0e2a208c20b6ee33b`.

Checks cover whole-song and lead-in frame totals against ordinary renderer
measurement, silent range pre-roll, a later stereo trigger (including pre-roll),
missing phase-preserving control support, invalid derived format, failure of
either workspace allocation, tick/frame budgets, unsupported EFx, later classic
repeat handoff and silent-tail segment playback. The existing injected16-voice
fixture still checks runtime failures, confirmed-stop ownership, cache lease
retention and exact24-bit enhanced saves.

During fixture development, the first pre-roll case ended at its start row
(empty range), and the first8-bit handoff case retained out-of-range24-bit values;
these invalid test inputs were corrected. Native GCC rejected nested zero
initialization under strict warnings; explicit memset preserves the same state.
No production or emulator failure/retry occurred after final validation.

This is capability analysis with immutable caller-owned inputs, not an owned
playback session, scheduler, native editor gate, cache capacity guarantee or
hardware acceptance. Analysis does not write masters/upload/operate devices.
Renderer validation may read PCM. Native PLAY remains unwired. AmiGUS endpoint,
stop-fence, capacity and physical output still need separate qualification.
