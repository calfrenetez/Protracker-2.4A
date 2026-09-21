# Live repeat-range preparation

`pt_voice_set_repeat` changes a live voice's next forward repeat range within
its existing immutable PCM. The current segment or loop iteration finishes at
its previous boundary; updating the range does not jump or restart its phase.
The last update wins if several arrive before the boundary. Interpolation at
the boundary blends into the newly selected repeat start. Position, step,
precision and interpolation setting are otherwise preserved.

This also lets a currently active one-shot acquire a forward repeat. Invalid,
empty or out-of-source ranges, inactive voices and pingpong voices are refused
without changing state. No PCM writes or allocation occur. The existing voice
contract requires valid initialized state and borrowed PCM throughout use.

Tests cover initial-segment updates, replacement before a boundary, updates
while already looping, one-shot interpolation across the new boundary and
invalid/inactive/pingpong refusal, alongside existing huge-step trajectories.

This is a foundation for reference repeat-register behavior. The renderer does
not yet call it or accept previously refused instrument-only sample/slice
handoffs. The original API describes only a range in the same source; the
additional borrowed-source API is described below. No new full
tracker-effect compatibility or physical audio acceptance is claimed.

Validation: the complete 72-test host suite passed. Native run `segment1789953011204947000` passed the updated segment and legacy mixer tests in 27.697 seconds, with identical host/native trajectory output and stopped DMA. Evidence is retained in `evidence/enhanced-editor/dev64`. Guarded emulator shutdown and fresh release checks completed.

## Borrowed next-source handoff (dev65)

`pt_voice_set_repeat_source` queues another immutable PCM for the next repeat
boundary. The current source and current iteration finish without a phase jump.
Boundary interpolation reads the new source's first repeat frame, then the
voice switches its borrowed source at the boundary. Any overshoot is folded
into the new repeat length. Further updates replace the pending source/range;
`pt_voice_set_repeat` can cancel a foreign-source handoff in favor of the
currently sounding source.

The pending source must have matching bit depth, channel count and sample rate.
This operation does not convert formats or change playback step. Both borrowed
source descriptions and PCM must remain valid and immutable while referenced.
Full source validation and voice/source alias checks precede mutation. Frame
and mix output checks protect pending data and metadata as well as current
data, even before the handoff occurs. No allocation or PCM writes occur.

Tests use different source lengths, verify interpolated forward and return
handoffs, pending-source cancellation, source immutability, incompatible-rate
and range refusals, and pending-source output alias refusal with unchanged
voice/output/clip counts. Existing segment trajectories and legacy mixing remain
in the suite. Renderer effect acceptance is still unchanged; pinned tracker
fixtures and integration are the next steps, not evidence established here.

Dev65 validation: all 72 host tests passed, as did the cross-build and native segment/legacy voice tests (`segment1789953593091072000`, 27.764 seconds). Native and host trajectory output matched. Evidence and guarded release checks are in `evidence/enhanced-editor/dev65`. No renderer integration or physical acceptance is claimed.

## Pinned active instrument-only fixtures (dev66)

Four two-sample MOD fixtures now capture looped handoff, return to the first
sample, handoff to a non-looping sample, and ED3 on an instrument-only row.
Each was run twice against the pinned sample-trace diagnostic; every trace byte
matched. The independent baseline's first 52 bytes per tick still match dev38.

The captures show the stored sample start/length, loop pointer/length and volume
change immediately at the instrument-only row. The trigger snapshot remains
the first sample at offset 2108, length 512 words, trigger count one throughout;
period stays 428. The second sample is at offset 4156. Its looping repeat is
[4220,4476), while its non-looping repeat is one word at 4156. ED3 without a
note does not generate a new trigger. Returning to the first instrument updates
the loop and volume again without retriggering. These properties are asserted
for every active tick, not just the row-transition snapshot.

The source's SetDMA writes each channel's stored loop pointer and repeat length
after processing a row, even with no newly triggered note (see pinned
`vendor/pt23f/PT2.3F.s`, SetDMA). These traces capture stored ranges and trigger
writes; they do not directly observe Paula's live fetch pointer or prove audible
PCM continuity. The next renderer comparison must model the current iteration
finishing before its new repeat source is used, and account for non-looping
first-word handling explicitly. Renderer refusal boundaries remain unchanged.

Native run `instrument1789954169827287000` passed nine executions in 37.299 seconds.
The new retained-evidence regression passed in 0.005 seconds; the previous
72-test suite remains the product-code baseline (no product changes here).
Evidence is under `evidence/enhanced-editor/dev66`. Emulator resources were
verified released and AmiConnect was notified. No physical acceptance claimed.
