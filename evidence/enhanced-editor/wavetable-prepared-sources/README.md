# Prepared wavetable sources — 27 September 2026

Private ready-song dispatch and range restore reuse the captured immutable
project and exact held master pins. Each source operation checks revision/header,
bridge/backend/reservation identity and live ownership. Upload acquires a separate
reference to the exact prepared version; location validates the current held
cache lease. Public APIs still fully validate; no persistent trust flag exists.
Synchronous selected-source PCM validation/conversion is unchanged.

Evidence:
- Instrumented real project-validator counter: no scans during ready song ticks,
  including uploads, cache-hit retriggers, controls and range restoration.
- Public bridge acquire/location/dispatch reject invalid unused PCM after private
  playback. Prepared capacity, pointer, metadata and current-token changes refuse
  before bus writes/start; original true24 data remains intact.
- Existing whole-batch restore, fractional cursor, uncertain start/restore/stop,
  stale identity/lost reservation, allocation/cancel/large promotion, edit guards
  and master-preserving save fixtures pass.
- Final ASan/UBSan instrumented wavetable suite PASS6.711s; staged editor wavetable,
  legacy edit guard and Studio ownership suite PASS27.335s.
- A new negative fixture initially expected DEVICE for invalid capacity; the
  renderer correctly refused earlier with RENDER and zero writes/starts/pins.
  Corrected assertion then passed all final host checks.
- Native build and staged editor-main syntax PASS.158 staged inputs/font verified.
  Binary323820bytes SHA256
  f107e1c948d589b1f66a311701c692ba88fe235b1b13ce25e7cf096145b9c982.
- Coordinated shared030/DevBench run render-files-1790485913974044000 RC0
  within90s:548 Fast allocations, zero owned bytes, no Chip fallback. All four
  DMA channels off, exact run-file cleanup verified, then explicitly released
  to AmiConnect. No recovery hold or physical operations.

This is software memory/ownership evidence with injected bus/library callbacks,
not native PLAY, real-time scheduling, physical AmiGUS or audible acceptance.
Existing packaged editor and unrelated display changes remain untouched.
