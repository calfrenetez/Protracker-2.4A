# dev37 — reference arpeggio

The shared WAV/sample-bounce renderer now accepts 0xy. Effect passes cycle between
the stored period and the high/low nibble table offsets, retaining PCM phase and
the stored base. Fresh rows, delayed tick zero, 000, inactive voices and prior
wrapped/non-table slides retain native period-register behavior.

Native high offsets can cross the zero-finetune table's zero sentinel into the
following tuning+1 table. The portable implementation stores those adjacent words
explicitly, avoiding out-of-bounds access. Zero playback periods remain refused
during measurement, before sink calls, file staging or sample append. Ordinary
notes still retain raw periods; finetune and other unfinished effects are refused.

## Validation

- 47 host checks passed with sanitizers: 46 regressions in 64.256 seconds plus
  the new arpeggio/native comparison in 5.516 seconds.
- Run `arp1789927626627766000`: 39 native executions passed in 154.068 seconds.
- Nine fixtures, 153 ticks per capture pass, repeated byte-for-byte. Host and
  m68k match every stored/output period word. The unchanged DMA/phase oracle
  checks every rendered true24 ramp frame against captured native periods and
  volume. The zero-period fixture refuses before sink/report mutation.
- Cases cover both nibbles, 000 transition, delayed rows, adjacent-table reads,
  zero output, prior slides/wrap, inactive voices, speed31 and four native tracks.
- Sanitized boundary checks cover independent parameters on all 16 tracks,
  selected-track isolation, zero-nibble lookup after a non-table slide, masked
  counter behavior and the full sentinel/adjacent-table offset range.
- The pitch diagnostic, original flow diagnostic and native Paula test binaries
  are unchanged from dev36. The baseline's first36 fields remain exact dev28.
- Build source hashes and native-tested binary identities were checked against
  `core-build.json`; shipping replay instrumentation remains separate.
- Main-screen golden remains
  `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.
- AmiConnect explicitly confirmed the window free before launch. Guarded cleanup
  and fresh process/HDF/socket checks at 18:10:06 UTC confirmed release and exact
  launcher restoration. Ownership was explicitly released to AmiConnect.

## Limits

This proves reference software register/PCM behavior. It does not establish
analogue Paula sound or physical AmiGUS/A1200 performance/listening acceptance.
No physical hardware was accessed. Vibrato, remaining effects, finetune, ordinary
note table quantization, cross-sample/slice glide handoff, selected-row bounce,
batch stems and enhanced mixed-backend playback remain open.

## Binary identities

PT24GEdit: 186676 bytes, SHA256 `7fc7b66b3adb07744c63f7d4f1fdfed3b652f470a0db33f02088047fd1a588f7`.

PT24GRender: 61960 bytes, SHA256 `9944fc3d0c8306a8a0c93473e6a50654c85868f3d69b4515cde88dce1e484c29`.

PTPitchTest: 53408 bytes, SHA256 `9d888a81279391fa1e969386ddff5faaecd6fcfa6dbbbf692d5edd81307cac5d`.

PTPortaRenderTest: 63948 bytes, SHA256 `091864cc33238198a1897e62fb1690d4da3e1ea10ab804d809db4aa79e4a7f4f`.
