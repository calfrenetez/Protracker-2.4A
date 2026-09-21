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
handoffs. Cross-sample repeat pointers need a separate borrowed-source model;
this API deliberately describes only a range in the same source. No new full
tracker-effect compatibility or physical audio acceptance is claimed.

Validation: the complete 72-test host suite passed. Native run `segment1789953011204947000` passed the updated segment and legacy mixer tests in 27.697 seconds, with identical host/native trajectory output and stopped DMA. Evidence is retained in `evidence/enhanced-editor/dev64`. Guarded emulator shutdown and fresh release checks completed.
