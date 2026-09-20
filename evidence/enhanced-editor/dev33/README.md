# dev33 — undoable render to new sample

NEW SAMPLE / U on the existing render page creates one new assignable sample
using the selected song/pattern, tracks, gain, lead-in, rate and precision.
The PCM is rendered directly into a single owned allocation within the sampler
budget. Completion appends the slot as one shared undo resource. Existing slots,
events and arrangement are preserved. No intermediate file or full PCM copy is
needed, and no implicit precision/rate conversion is applied.

## Validation

- `host-tests.log`: all 43 host checks passed in 51.428 seconds.
- The new bounce test checks every true24 stereo sample value, allocation failure
  at each staging allocation, memory limits, cancellation during analysis/mix
  and at the final callback, report preservation, redo retention, mixed note/
  sample undo, referenced-slot refusal, table reuse, journal eviction, exact PTG
  round trips and complete allocator release under address/undefined sanitizers.
- Existing slot/sampler/editor regression tests passed. The accepted main-screen
  golden remains `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.
- Native run `bounce1789921643260694000` passed in 100.516 seconds, including the
  native core test and editor cancellation/undo/redo/save/reopen workflow.
- The new slot contains 36480 stereo24 frames at 48000 Hz. Every encoded
  PCM byte matches the host reference WAV. The project differs only in sample
  count and the appended named sample; existing chunks are unchanged.
- Cancelling a second bounce after undo preserves redo of the completed bounce.
  Reopening and saving the result is byte-identical. Audio DMA is off at checks.
- `native/bounced-sample.png` was inspected: selected slot 32, its name, stereo
  waveform, 24-bit/48000 Hz format and full frame range are visible.
- `core-build.json` pins source/compiler/runtime/binary identities. A comment-only
  test clarification was rebuilt; tested and packaged binary hashes match.
- `amiberry-release.json` confirms no emulator process, private-HDF holder or
  socket after guarded shutdown and launch restoration. Ownership was explicitly
  released to the AmiConnect thread.

PT24GEdit: 185100 bytes, SHA256
`5a619dc045fa7f9c768528737a600fb735628c61ab5acea6b51fb4bfdd130f31`.
PTBounceTest: SHA256 `5e37468b2e626d55b109feb91d8a27d2e5d5076e3f0ce85bfabedb419288744c`.

The first native run passed all data checks but its screenshot retained sample31
following undo/redo. Only test navigation was changed to select the bounced slot;
the repeat passed with the same native binary and a verified screenshot.

## Limits

The shared renderer's supported-effect subset and ideal-BPM/PCM-rate reference
profiles still apply. Selected-row bounce and batch stems remain unfinished.
High-resolution Paula audition is unsupported. This is software/emulator proof,
not physical AmiGUS/ACA1234 performance or listening acceptance. No physical
hardware was accessed.
