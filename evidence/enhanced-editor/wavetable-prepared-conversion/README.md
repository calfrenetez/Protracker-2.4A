# Prepared wavetable conversion — 27 September 2026

Only the immutable prepared wavetable song uses private cache/upload entrypoints,
while retaining the exact master version. Shape/capacity/format/overflow/alias and
live ownership checks remain. Shared conversion/publication/failure cleanup code
omits redundant value validation; public APIs retain it even on hits. No persistent
trusted cache mode or mutable EFx bypass exists.8/16/24-bit masters remain intact.

Validation:
- Instrumented project and PCM validators: no calls during prepared song ticks,
  uploads, hits, controls or range restoration; exact cursor/ownership fixtures
  retained. Final ASan/UBSan wavetable suite6.730s PASS.
- Public/prepared chunked upload parity across8/16/24-bit sources,8/16-bit output,
  stereo selection, endian, word padding and varied chunk capacities. Partial
  transfer and alias/capacity cleanup remain atomic. PASS0.392s.
- Public/prepared AmiGUS cache lifecycle, capacity/invalidation/pinned-old-data,
  partial transfer, lost ownership latch and refusal after detach PASS0.452s.
  Private bad shape/format preserves lease outputs and active data; public invalid
  values refuse on a populated cache after private load. A test initially used an
  incorrect enum-order assertion; corrected to exact CAPACITY/INVALID results.
- Public playback conversion PASS0.398s; mutable sampler EFx PASS5.203s.
- Staged editor wavetable/legacy guards/Studio ownership PASS27.439s.
- Native build/main syntax PASS;159 staged inputs/font verified. Binary326152bytes
  SHA256 9c619156d11f59c9553308213df5fc37bdc19d28af6114c1a46f9bde0e05e436.
- Coordinated shared030/DevBench run render-files-1790486330874192000 RC0
  within90s.548 Fast allocations, zero owned bytes/no Chip fallback. Verified
  all four DMA channels off and exact run files cleaned; explicitly released
  to AmiConnect afterward. No recovery hold or physical operations.

First-use uploads still loop synchronously through bounded staging/write chunks.
This removes redundant validators, not the work required to convert/upload bytes.
No native PLAY/clock/transport, hard real-time, audible or physical acceptance.
Existing native editor package and unrelated display work unchanged.
