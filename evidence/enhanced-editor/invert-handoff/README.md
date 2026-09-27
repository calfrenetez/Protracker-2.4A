# EFx instrument-handoff reference and native core qualification

2026-09-27. Production change: explicit private EFx rendering now accepts compatible
same-rate classic instrument-only handoffs, including one-shot masters with
nonzero first words. The private copy alone receives the classic cleared word.
Ordinary renderer admission and original master data remain unchanged.

## Reference captures

`invert-shared-1790466636428868000`: four bounded MOD fixtures captured twice by
PTInvertTraceTest on shared030. Every run returned0/native-stop after24 ticks.
The diagnostic uses the pinned 2.3F replayer; `diagnostic-build.json` records build
inputs, compiler/runtime and binary identity. No replayer semantics were changed.

Fixtures cover a same-row speed/instrument change, inherited EF8 clock, one-shot
handoff with EFf, and one-shot handoff with EF0, followed by return to instrument1.
Period381 forces fractional phase and at least one repeat handoff inside a tick.
The first-word bytes17,163 of sample2 are deliberately nonzero in each MOD.

Raw `.log` files preserve independently allocated Paula sample addresses. `.trace`
files map pointers back to per-sample MOD offsets; no scalar clocks or PCM bytes
are masked. Both captures compare exactly after this mapping. The reference
trace records the entire16-byte loop (or two-byte one-shot repeat) for channel0.

## Host and native checks

- Host sanitizer suite:33 test invocations PASS in56.499s, covering ordinary
  instrument handoffs, EFx rendering/files/stems/queued sessions, sampler ownership
  and editor ownership. The new handoff test was included twice by overlapping
  discovery patterns.
- Final trace identity/relocation/clock checks:4 tests PASS in2.768s, including
  fixture hashes, both raw captures, per-sample clock/cursor/mutated-byte agreement
  and immutable masters.
- New PCM oracle compares every stereo24 frame for offline and incremental
  pulls1/17/256; period, volume, repeat pointers and mutation snapshots come from
  the reference captures. It does not call production EFx mutation code. Each case
  emits17280 frames, switches repeat sources twice, and preserves both masters.
- Rate mismatch, interpolation and16/24-bit private-EFx sources refuse before
  output; original non-EFx admission still refuses EFx.
- Clean staged-source snapshot `build/dev/invert-handoff-candidate-a07d10f` excludes
  unrelated display edits. Editor EFx ownership sanitizer test PASS in7.453s.
- Native `PTExecInvertHandoffTest` uses canonical m68000/nix20/-fbbb=- build flags.
  `native/build.json` records exact source/header/compiler/runtime/binary hashes.
  All four cases RC0 with exact PCM,41 Fast/not-Chip allocations and zero final
  owned bytes; the real allocator also refuses a zero-budget allocation.

The native oracle exceeded its initial120-second capture deadline. `native/result.json`
retains that failed deadline. `native/completion-result.json` records later functional
completion during a coordinated passive extension of the same finite run. No retry,
reset, additional launch or configuration change occurred. Do not interpret the
later pass as a timing/realtime acceptance. The reusable runner now requires a
420-second reservation for this deliberately expensive one-frame pull oracle.

Both windows ended with all four DMA channels off, guarded exact run/launch cleanup,
verified absence and explicit release to AmiConnect and Scott. No native editor
window, AmiGUS device output, audible playback or physical acceptance is claimed.
