# Filtered sample conversion, dev16

FORMAT defaults to a fixed-point Blackman-windowed sinc filter; F switches to
linear conversion. Filtering preserves mono/stereo and 8/16/24-bit precision.
Native progress reports percentage and accepts Escape; cancellation discards the
staged version without modifying source PCM, project revision or redo history.

## Evidence

- All 25 host groups pass, including ASan/UBSan, generated-table reproducibility,
  integer/fractional rate passbands, alias rejection, stereo24 DC, endpoint and
  capacity checks, progress cancellation, exact undo and allocation rollback.
- Native PTFilterTest and PTSamplerTest pass. The native editor imports a mixed
  1 kHz + 6 kHz sample at 32 kHz, cancels an 8 kHz conversion, exports the exact
  original bytes, redoes the preceding edit and then completes filtering.
- Independent host analysis finds maximum interior error of 1 LSB against the
  wanted 1 kHz component. Exported WAV and PTG PCM match, CRC validates, reopening
  and resaving is byte-identical, both editor exits are normal and DMA is off.
- The complete native run took 215.763 seconds; the conversion workflow took
  19.601 seconds including automation. This is not a hardware speed measurement.
- The first native attempt exceeded its 180-second unit-test deadline. Reusing
  integer-phase kernels and removing per-tap 64-bit division resolved this for
  the retained final build. The unsuccessful attempt is not counted as a pass.
- Screenshots are actual native output. The accepted main-screen golden is
  unchanged. core-build.json records source, compiler and binary identities.

## Limits

The filter uses no FPU or heap allocation internally, with approximately 16 KiB
stack for weights; launch from a Shell with Stack 65536. Reductions greater than
128:1 refuse before publication; use an intermediate rate or explicit linear mode.
Precision reduction has no dither. Source endpoints are extended and filter
overshoot saturates to the sample precision. These synthetic checks do not prove
subjective audio quality or physical ACA1234 performance. AmiGUS hardware,
enhanced playback and MIDI remain unvalidated or unimplemented as documented.
