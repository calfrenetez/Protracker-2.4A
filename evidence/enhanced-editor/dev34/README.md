# dev34 — reference fine-volume slides and note cut

EAx, EBx and ECx now work in the shared reference renderer used by the WAV
utility, native editor export and undoable sample bounce. Fine slides saturate
at 0/64 and apply on tick zero, including delayed passes without refetching the
instrument. EC0 cuts immediately; other cuts compare the actual current tick.
An unreachable cut does not fire. Volume restoration reveals continuing sample
phase rather than retriggering or leaving a stopped voice.

## Evidence

- All 44 host checks passed: 43 existing regressions in 58.007 seconds and the
  new native-trace/phase comparison in 2.309 seconds, with sanitizers.
- Final native run `volume1789923338464833000` passed all 22 executions in
  141.895 seconds. Seven fixtures contribute 148 ticks each capture pass,
  repeated exactly, and 101760 constant-source PCM frames per renderer platform.
- `core-build.json` pins source/compiler/runtime/binary identities; the packaged
  editor and renderer remained byte-identical through the test-only adjustment.
- `amiberry-release.json` verifies guarded cleanup and launcher restoration.
  Ownership was explicitly released to AmiConnect after the final run.
- Main-screen golden SHA256 remains
  `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.

The seven synthetic MODs cover upward/downward saturation, zero parameters,
effect-only rows, immediate/delayed/unreachable cuts, delayed tick-zero slides,
delayed cuts and speed one. Every constant-source rendered stereo16 frame is
compared against raw-volume records from the pinned original replay. The
separate native diagnostic binary is unchanged from dev28. Captures are repeated
from reset state and all 36 bytes of every record must match.

A separate true24 seven-frame ramp checks silent phase continuity for all
16 voices together, then isolates track 16 through the upper selection-mask bit.
The regression suite covers retained WAV publication, bounce undo/cancellation,
project persistence and the unchanged accepted main-screen golden.

The first emulator window completed the original exhaustive per-track phase test,
but was deliberately interrupted to shorten repetitive test work. Cleanup was
verified. The final harness checks all voices together with a separate upper-bit
selection pass. Production binaries did not change during this adjustment.
The interrupted window is not counted as a completed fixture run.

## Limits

This isolates volume behavior, not analogue Paula output. The reference clock
remains ideal BPM and pitch remains PCM-rate-at-period428. Finetune, the remaining
classic effects, selected-row bounce, batch stems and enhanced live playback
remain unfinished. Unsupported commands are still refused before publication.
No physical hardware was accessed; performance and listening acceptance remain
open. The main-screen layout, pinned replay and shared undo behavior are unchanged.

## Packaged binary identities

PT24GEdit: 185236 bytes, SHA256 `37a379ed0082e8d4585576ba237e42bc61f3770ed5f1e2b39fe00eb9cffb2af2`.

PT24GRender: 60520 bytes, SHA256 `6eb1902f2b5f03e1de7ebce07077d80cd226a6397591f5673ea181d20461c67f`.

PTVolumeRenderTest: 60584 bytes, SHA256 `f47dfb89c2461a5aa1ee3dd07b93ede7b25def98d7b90ffffb30eac3ab0f8d68`.
