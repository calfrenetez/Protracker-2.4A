# Studio preparation before output — 27 September 2026

Ordinary Studio now supports cancellable sequence analysis and selective master
promotion before output. Core begin/prepare performs one measurement chunk,
interval transition or <=256-frame silent phase chunk. No provider callbacks or
PCM are emitted during analysis. The completed sequence is reset without a second
measurement, retaining the required mask including pre-roll, segment and repeat
sources. Sampler preparation reserves one required version or copies <=4096 bytes
per call; all required pins are held before readiness. Public provider validation
remains intact. Existing synchronous open/start drives this same state machine.

Editor begin/prepare/begin_queued retains the existing change guard while pending.
A queued pump advances preparation without publishing audio early. Stop/error/end
releases jobs and session pins, preserving independent consumer leases. Completed
unchanged masters can remain sampler-owned; unused slots are not promoted.

Coverage:
- Cancellation before/during source copying; stale generation/header refusal,
  budget refusal and full allocation-failure sweep with no leaked ownership.
- No output or partial master publication while pending; ready playback requires
  no new master allocation, verified with allocator refusal at first trigger.
- Selective pre-roll/repeat pins, unused slots untouched, exact stereo24/pingpong/
  slice and classic8-bit repeat PCM parity against offline reference.
- Core readiness gates, source mask, partition parity and provider failure;
  editor pending edit/Stop cancellation, queued edit/undo/Stop held leases,
  disposal/output shutdown and natural draining.
- Mutable EFx sampler/private-bank regression remains passing and separate.

Validation:
- Final sampler song ASan/UBSan PASS5.196s; core song PASS3.213s;
  sampler EFx song PASS5.284s; queue pump PASS0.510s.
- Staged editor wavetable/legacy guard/Studio lifetime suite PASS27.140s.
- Native build/main syntax PASS;155 staged inputs/font verified.
  Binary315496bytes SHA256
  b4295a1058d3502aa362afd704787446a9ee535257ba487365df95b86b365d42.
- Fresh AmiConnect confirmation, other-task idle snapshots and shared lock/live
  guards: render-files-1790484344082947000 RC0 within90s.477 Fast allocations,
  zero owned bytes/no Chip fallback. All4DMAoff/exact cleanup verified; explicitly
  released shared030/DevBench with no recovery hold.

Limits: initial/reset validation, allocation and public provider/voice validation
remain synchronous. Bounded copying is not elapsed-time or real-time acceptance.
No native PLAY/clock/card transport, audible or physical acceptance. Existing
native editor package and unrelated display/harness changes preserved.
