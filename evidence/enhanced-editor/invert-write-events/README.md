# Actual EFx write events for longer loops

The separate pinned-replayer diagnostic records each actual EFx byte store in
execution order. Its208-byte tick record retains the existing140-byte replay
state, then a count/overflow header and eight address/value/channel entries.
The bounded ISR buffer performs no allocation; overflow is a failed capture.
The shipping replay and previous164/188-byte diagnostics are unchanged.

Four fixtures each repeat exactly: the previous16-byte shared-bank baseline,
and18/64/256-byte loops with shared-clock collisions and instrument changes.
The18-byte case wraps; the longer cases write beyond the former16-byte snapshot.
The baseline's replay state and reconstructed bank snapshots agree with its
previous188-byte capture. This does not claim all loop sizes or effect mixtures.

The host oracle replays logged stores into independent sample copies, checking
each complement and every byte of both private banks after each active tick.
A separate PCM oracle checks every stereo24 frame offline and in1/17/256-frame
pulls, including excluded/muted/soloed mutators. The original master samples stay
unchanged. Long cases produce40320 frames; baseline17280. Actual WAV,
individual/grouped stems and bounce match the same reference; cancellation,
undo/redo and enhanced-project save/reload preserve masters. Corrupt addresses,
values and channel masks remain visible; overflow/count/unused-slot corruption
is rejected. No production effect admission changed.

Reference run: invert-shared-1790470311250241000 (90-second bound), eight RC0
native-stop captures. Result records DMA-off and exact owned cleanup. Build
identity is in diagnostic-build.json; raw logs and normalized traces are retained.

Reproduce host checks: `python3 -m unittest discover -s tests -p 'test_invert*.py'`.
Build reference diagnostic with `tools/build_shared_invert_trace.py --writes
--cc <pinned compiler>`. Native PCM target: PTExecInvertWriteTest; coordinated
runner: `tools/shared_infra_invert_handoff.py --writes`. Reserve the shared guest
and use the shared lock/live guards. Native checks use17/256-frame pulls; the
one-frame stress partition is host-only for this longer-loop milestone.

These are software/reference and memory-ownership checks. Physical Paula sound,
68030 realtime performance, A1200 endurance and AmiGUS hardware remain separate.
Native Studio output is not enabled; private EFx still refuses16/24-bit masters.

Host regression:19 tests PASS in38.723s with address/undefined sanitizers.
Native run invert-handoff-1790470690753871000 missed its420-second batch deadline;
`native/deadline-result.json` preserves that failure. During a separately
coordinated120-second read-only observation period the existing script completed
naturally at451.8 seconds, all four RC0. This is not an in-bound batch pass.
Offline/pull17/256 plus mute/solo checks passed; each case made44 Fast allocations,
left zero owned bytes and refused the test budget without Chip fallback.
`native/completion.json` records completion, all four DMA off and guarded exact
cleanup; ownership and recovery hold were explicitly released. No retry/reset.
The native log repeats17 in its partition label; duplicate17 execution is skipped.

Native binary: df8df55fe28e32e4ec362f8e14431859dca2a7de4d12f6ccc151aa984dfc924f,
89772 bytes. All45 dependency identities are in native/build.json; production
sources match committed application bytes. Future native batches should be split
or sized from this measured duration before reserving the guest.
