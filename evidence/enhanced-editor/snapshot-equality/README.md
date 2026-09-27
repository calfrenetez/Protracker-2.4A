# Byte-exact snapshot word comparison — 27 September 2026

The project guards in wavetable and direct-master Studio song owners still compare
every descriptor byte, including metadata, padding and the short tail. Channel
selection retains its existing UI-only normalization. Every original comparison
site and all generation/master/cache/ownership checks remain. The alias-safe
memcpy-to-uint32 helper uses the native project's two-byte ABI alignment; other
targets make no extra alignment assumption. No borrowed-array/PCM validation
contract is changed. Native assembly now compares376longwords then two bytes,
instead of the1506byte-loop comparison in the previous snapshot guard.

Host ASan/UBSan dispatcher PASS8.155s including mutation of EVERY object byte in
both operand directions, equality after restoration, self-alias and unchanged
inputs/canaries. Studio owner PASS5.185s; staged editor lifecycle PASS29.097s.
Native build/editor-main syntax PASS.174input/font/index hashes verified;
411232byte candidate SHA256
d0bf5ff107df640a8d86180c6a64ec870194b1ebdf419d041fab7681f5d60f90.

Shared030 run render-files-1790510080767337000 passed within coordinated120secs,
RC0. Native byte/tail/alignment and timer/watchdog/editor/lease assertions passed;
1255Fast allocations ended with zero owned bytes/noChipfallback. Running-state
checks passed beforestaging and beforelaunch; all4DMAoff before exactcleanup,
independentabsence verified; explicitrelease to AmiConnect. No recoveryhold.

Observed repeated readiness748..757EClock ticks at709379Hz, startup4440ticks for
onevoice and26989ticks for16. The previous recovered candidate observed9750 and
68945ticks respectively. These observations are consistent with less guard work,
but include emulator scheduling/instrumentation and do not qualify realtime or
physical speed. All no-late/uncertain-stop rules remain. No physical/card/audio
operations or native enhanced PLAY enable.
