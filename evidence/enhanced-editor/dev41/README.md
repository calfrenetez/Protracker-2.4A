# dev41 — reference sample-offset rendering

9xx is enabled in the shared WAV/sample-bounce renderer for a bounded mono8
subset. Per-track offset memory, saved range and initial-trigger range preserve
the dev39 native semantics: double application on note rows, single application
on no-note rows, 900 memory, reloads, range inheritance and two-frame fallback
without pointer advancement at/beyond length. Delayed passes do not reapply it.
Nonfresh 9xx restores the stored period. Initial ranges hand off to independent
forward loops through the dev40 primitive.

Tracks using 9xx in selected scope require mono8 samples of even lengths from
2 to131070 frames, no loop or even-boundary forward loops, and no slice notes.
Other formats/offset semantics are refused before any sink or publication.
Measurement and streaming share interpretation and range bounds checks.

## Validation

- All 51 host checks passed in 84.341 seconds with sanitizer coverage.
- Final native run `offsetpcm1789931520000733000`: 12 checks passed in 150.299
  seconds, with output identical to host checks.
- Eight retained native range fixtures drive 11 independent channel PCM checks:
  117120 stereo24 output frames match captured initial/loop ranges exactly under
  the immutable reference policy. The four-channel case checks every channel.
- Safety also checks isolated track16 output and non-byte/stereo/odd/pingpong/
  slice refusal without sink/report mutation.
- Existing portamento forced-retrigger mutation still fails as expected; its
  anchor now selects the actual trigger branch rather than the new range branch.
- Initial host validation exposed stale loop metadata in a stereo-refusal test;
  this test-only setup was corrected before the final full run.
- A rejected native attempt encountered an output/input filename collision in
  the private harness copy. Retained dev39 evidence remained intact. The driver
  now asserts distinct names before launch and stops on nonzero test returns.
  Rejected-run log/assertion/cleanup are preserved separately. Accepted evidence
  comes only from the corrected run; production binaries were unchanged.
- Build inputs, native-test identity and all retained input hashes are verified.
- Guarded cleanup at 19:14:58 UTC verified no process/private-HDF holder/socket
  and exact launcher restoration. AmiConnect received explicit release.

## Limits

The source PCM remains immutable. Nonloops stop after the initial segment;
native Paula's first-word clearing and repeated silent guard are not reproduced.
This validates reference PCM using native range decisions, not analogue sound.
Stereo/high-precision/slice offset behavior, remaining effects, finetune and
mixed-backend playback remain unfinished. The accepted screen is unchanged.
No physical hardware was accessed or accepted.

## Binary identities

PT24GEdit: 188344 bytes, SHA256 `c6dd5cd5430f1b6bae3f413146a946253257ab419591ab28e68875e7602bbe94`.

PT24GRender: 63628 bytes, SHA256 `c38b21efc2448d6acc0dc79bc2754deed32e98a807572537e8ca8fe728928e99`.

PTOffsetRenderTest: 64628 bytes, SHA256 `d2237415db3ae9296897a8da8feef2452de0eadd6a8f80f740382ee595d659b6`.
