# Immutable PCM voice and wide mixer, dev30

`src/core/voice.c` adds bounded, allocation-free PCM traversal and mixing. It
borrows validated immutable mono/stereo 8/16/24-bit sources, accepts explicit
ranges and positive Q32 source-frame steps, supports forward/ping-pong loops,
and supplies nearest or linear interpolation. Ping-pong endpoints are not
repeated; one-frame loops stay constant. Forward-loop interpolation wraps across
the correct seam. Large steps are reduced with bounded modular arithmetic.

The mixer accepts sixteen initialized voices and caller-specified per-side Q16
gains. Zero gains preserve voice advancement. Signed24 values accumulate in
signed64 before final round/quantize/saturation to stereo16 or stereo24. It counts
clipped output values and performs no automatic normalization/dither. Capacity,
gain and alias preflight errors preserve voice state and output. No PCM mutation
or per-frame allocation occurs. PCM and descriptor lifetime remain caller-owned.

All 40 host checks pass in 43.947 seconds. Hand-calculated cases cover ranges,
normal/fractional/large steps, forward and ping-pong seams, one-frame loops,
8/16-bit sign scaling, asymmetric true24 stereo, rounding/clipping, sixteen voices,
muted advancement, empty voices, capacity and alias failures. An independent
Python unbounded-integer trajectory oracle agrees on all 2048 mixed output values
via their fixed hash `990c42f0` and 40 clipping events. Its cases include a
UINT64_MAX step and mirrored fractional playback. Small irregular blocks and one
large block produce identical PCM, clipping counts and final voice state. Source
PCM remains unchanged. Host code runs under ASan/UBSan and warnings-as-errors.

Native run `voice1789911318607653000` passed in 27.405 seconds. The 68000-compatible
`PTVoiceTest` runs on the emulated 68030 and produces exactly the host log/hash.
Its SHA-256 is `ffe54a63d3a86094a6759d63d8806ff87da8b2790362a877492e0a228d2f839d`.
Source/compiler/runtime/binary identities are retained in `core-build.json`.
No audio device is started by the test; all DMA channels are off. Private launch
restoration, guarded shutdown and fresh process/HDF/socket checks prove release
to AmiConnect at the recorded timestamp.

The native editor remains byte-identical to dev27..29:
`dfa0b3c42c4099705650a0a279711b6e25e7ef0e3e791870c56bac1ab9685888`.
The main-screen visual golden is unchanged. Reproduce with `make test`,
`make core-tests`, then reserve the emulator before `tools/test_voice_emulator.py`.

This is reference PCM infrastructure, not finished song rendering, a selected
pan/routing law, ProTracker effect/period parity, high-resolution hardware audition,
physical AmiGUS sound or real 68030 performance. Linear interpolation is not an
antialias filter. Tracker sample/instrument/slice resolution, pitch/effects and
transactional WAV/bounce publication are subsequent integration work. No physical
A1200/AmiGUS was accessed.
