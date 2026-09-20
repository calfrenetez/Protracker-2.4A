# dev36 — tone portamento and combined volume sliding

3xx now glides toward a target without restarting the PCM voice. Nonzero speeds
are latched on effect passes; 300 reuses each track's speed. 5xx uses that glide
speed while applying its own volume slide. Target arrival clears the target but
retains speed memory. Delayed passes, speed-one latching, old targets after a
normal note and signed comparisons of wrapped stored words follow native replay.

Tone targets use the original zero-finetune table and its zero sentinel. Ordinary
reference notes retain raw periods. Repeating the same instrument resets volume
without retriggering. Glides before a normal note do not start a voice. Explicit
project velocity updates without triggering. Cross-sample glide handoff and
slice-target glides are refused before output rather than approximated.

## Evidence

- All 46 host checks passed: 45 regressions in 57.785 seconds and the new native
  portamento/PCM/phase comparison in 5.925 seconds, with sanitizers.
- Final native run `porta1789926414126669000` passed 39 executions in 146.860 seconds.
- Nine synthetic fixtures provide 149 captured ticks per pass, repeated exactly.
  Host and m68k pitch state match every stored/output period word.
- Every rendered true24 ramp frame matches an independent phase calculation
  driven by native output periods, volume writes and fresh DMA-start masks.
  It distinguishes inactive voices, normal triggers and continuing glides.
- All 16 tracks retain independent speed memory, reach targets and reverse
  direction. Cross-sample and slice-glide refusals preserve sink/report state.
- The same-instrument fixture starts at period404 so its phase is between loop
  boundaries at the glide. The host test rejects a deliberately retriggering
  renderer, proving this phase check detects that regression.
- The first native run was deliberately interrupted to strengthen that fixture;
  it is not counted as a completed run. Guarded cleanup was verified. Production
  and test binaries were unchanged for the final repeat.
- The 52-byte diagnostic, original flow diagnostic and native Paula test binary
  are unchanged from dev35. The old baseline's first36 fields remain exact.
- `core-build.json` pins current source/compiler/runtime/binary identities.
- Main-screen golden remains
  `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.
- `amiberry-release.json` verifies guarded shutdown, released process/private HDF/
  socket and restored launcher. Ownership was explicitly handed to AmiConnect
  for its requested next window; ProTracker will hold launches until release.

## Limits

Cross-sample and slice glides, glissando, nonzero finetune, zero playback periods
and remaining effects are still unsupported. Raw ordinary-note periods, ideal
BPM timing and PCM-rate-at-period428 remain the reference policy. This establishes
software register/PCM behavior, not analogue Paula sound or physical AmiGUS/A1200
performance/listening acceptance. No physical hardware was accessed.

## Binary identities

PT24GEdit: 186556 bytes, SHA256 `655cfac85c9956848607812d8bcbc490a9f0e8739c79f74b1c1eccf5570ce49a`.

PT24GRender: 61840 bytes, SHA256 `890e403f394f87377cadbcd13b0844f1c568726e6663d847ba6a33e36c80615d`.

PTPitchTest: 53268 bytes, SHA256 `b8b3bd510713520ba4bdc52d69551b75fd3e049f46f68cd7a61c46350c5e695d`.

PTPortaRenderTest: 63828 bytes, SHA256 `8cdd52cf711ba1fec1c54505f3e359d131efba7e37f2c6185c1950c3c8fa4857`.
