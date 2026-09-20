# dev38 — reference vibrato and waveform controls

The shared WAV/sample-bounce renderer accepts 4xy vibrato, 6xy vibrato plus
volume slide and E4x waveform/reset control. Nonzero speed and depth nibbles
latch independently on effect passes; 6xy retains that memory. Sine, ramp and
both native square selections reproduce byte-phase progression and word output
arithmetic. Stored pitch and PCM phase remain intact during modulation.

Ordinary notes reset vibrato phase unless the previous control disables reset.
The old control is checked before same-row E4x is applied. Tone-portamento notes
retain phase. Fresh speed-one parameters, delayed tick zero, blank effects and
inactive voices follow native register writes. Zero playback periods and
unfinished effects remain explicit preflight refusals.

## Validation

- 48 host checks passed with sanitizers: 47 regressions in 69.106 seconds plus
  the new vibrato/native comparison in 5.345 seconds.
- Run `vib1789928528667825000`: 47 native executions passed in 304.231 seconds.
- Eleven fixtures supply 286 ticks per capture pass, repeated byte-for-byte.
  Host and m68k match every stored/output period word. Every rendered true24
  ramp frame matches the shared oracle driven by native period writes, volume
  and fresh DMA-start masks.
- Cases cover sine/ramp/square, independent speed/depth memory, 6xy volume and
  memory, reset/no-reset and same-row control order, tone interaction, delay,
  speed-one latching, inactive voices and previously wrapped stored periods.
- Sanitized host boundaries cover all 16 tracks, all waveform-control nibbles,
  representative positive/negative phases, independent command memory, combined
  effect memory, reset ordering and 16-bit modulation wrap.
- The pitch diagnostic, flow diagnostic and native Paula test binaries are
  unchanged from dev37. The baseline's first36 fields remain exact dev28.
- Source/build identities and native-tested binary hashes match `core-build.json`.
- Main-screen golden remains
  `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.
- AmiConnect explicitly confirmed the window free. The cycle-exact PCM run took
  longer than estimated; coordination was updated while it remained bounded.
  Guarded cleanup and fresh process/HDF/socket checks at 18:27:37 UTC confirmed
  release and exact launcher restoration. Ownership was explicitly released.

## Limits

This proves reference software register/PCM behavior, not analogue Paula sound
or physical AmiGUS/A1200 performance/listening acceptance. No physical hardware
was accessed. Sample offset, retrigger/note delay, remaining effects, finetune,
ordinary-note table quantization, cross-sample/slice glide handoffs, selected-row
bounce, batch stems and enhanced mixed-backend playback remain unfinished.

## Binary identities

PT24GEdit: 186940 bytes, SHA256 `7d277c4729bf6836875e973faaa99e21085570e3614775cf04cb065d2f582ca0`.

PT24GRender: 62224 bytes, SHA256 `51ed03862f12b239046a78e7f88402a325fd69d4396edb240117306b288d73f2`.

PTPitchTest: 53660 bytes, SHA256 `1d70a0c1ac30ca25b4a9a1470e0802bc8b9d8d2adfdc08bf76e2423ae73e9431`.

PTPortaRenderTest: 64196 bytes, SHA256 `f3a8ba2b7198d4a249f4f75f445295548868e187a25914fd138e016cf713eabc`.
