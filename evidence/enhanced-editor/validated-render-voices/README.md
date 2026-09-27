# Validated immutable renderer voices — 27 September 2026

Ordinary stream/sequence renderers reuse complete initial project/source-value
validation for trigger, segment and repeat-source setup. Private voice entry
points retain shared bounded descriptor format/rate/capacity, geometry and alias
checks, omitting only the full value scan. Public voice/command entry points and
mutable EFx playback still validate source values. No PCM writes, allocations,
pins, cache changes or new ownership occur in the optimized setup.

Sources/options stay immutable and live through the render, including callbacks;
opaque sequences copy options but borrow project/PCM. This private API is not a
validation token for arbitrary data. Invalid untrusted sources must enter through
the fully validating public APIs. Source promotion preserves identical values.

Checks:
- Voice ASan/UBSan PASS0.725s, including exact8/16/24-bit loop/segment/handoff
  trajectories, bounded capacity/format/range/alias refusals without mutation and
  invalid public PCM rejection. Independent mixer trajectory hash remains
  990c42f0 with40 clips.
- Final instrumented render sequence ASan/UBSan PASS3.219s: exact24-bit audio,
  snapshot/restore, forward/pingpong, ranges, tempo/delay and partitions; complete
  calls do not invoke full PCM validation. Test-only translation-unit wrapper;
  no production hooks. Invalid mutable private data still fails at command setup.
- Private EFx queued session ASan/UBSan PASS3.682s.
- Eight PCM/voice segment/advance/render/volume/range/plan/EFx-handoff tests
  PASS24.229s, including stored pinned-reference and host/m68k-runner parity.
  These host/replay checks do not establish hardware timing or audible acceptance.
- Staged editor wavetable/legacy guard/Studio lifetime suites PASS26.371s.
- Native build and staged native editor syntax PASS. All152 dependency hashes
  match staged sources; generated font verified. Binary301940bytes SHA256
  35b0ec9ebd7dc7d649912b23fc97c43bda964c66d208118be98257497124406b.
- Fresh AmiConnect coordination/current idle other-task snapshots, shared lock
  and live identity guards: render-files-1790482992695410000 RC0 within90s.
  New validated-voice fixture and editor lifetime cases pass with334 Fast
  allocations, zero owned bytes/no Chip fallback. All4DMAoff, exact cleanup and
  explicit release verified. No recovery hold.

Limits: initial/reset validation, Studio source acquisition, backend conversion
and whole-master promotion remain synchronous. No hard real-time guarantee,
native PLAY/clock/card transport, audible or physical AmiGUS acceptance. Existing
native editor package and unrelated working-tree display/harness edits unchanged.
