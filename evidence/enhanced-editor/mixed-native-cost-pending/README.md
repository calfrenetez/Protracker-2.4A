# Mixed native cost fixture — corrected host/build, native pending

30 September 2026. Native run1790801822251988000 FAILED:90-second fixture timeout,
no done/test.rc, assertion at preparation of first16voice case. Sourcea24157b6dba29a6752045cf244bc7d449bb0ba3b,
native226752 SHA2563e005531343da9fa865c965bea077cf1eab3a7083167cbee0cd394bb23b9f4b4.
New fixture incorrectly forced all four Paula pans left; production capability gate
rejected incompatible physical slot stereo geometry as designed. No production
bug demonstrated. Only the8bit two-voice case completed before the assertion.
Its partial duration observations are FAILED-run diagnostics, never acceptance.
The run was not retried, reset, resumed or cleaned. No zero-owned-allocation or
counter/process closure proof after abort. Guarded read-only inspection found
running/DMAoff, retained exact run+launcher and unhelpful task names. DMAoff does
not establish resource release. Shared030/DevBench RECOVERY HOLD remains active.

Corrected fixture uses classic Paula L/R/R/L slot panning. Added full host coverage
for2voice and4Paula+12AmiGUS startup at8/16/24bits; no allocations/uploads at exact
commit and preserved global action order. Full ASan/UBSan host PASS7.789s:174cases
(existing168 plus6geometry/commit cases). Corrected selected portable/native+guard
build PASS162sources/zero generated/3verified binaries; hashes recorded. Corrected
candidate has NOT run in emulator. Native cost/timing/output acceptance pending.

Future cost scope: real EClock duration observations, injected logical song time
and voice/bus callbacks; no alarm submissions/MMIO/DMA/audio or physical access.
Measure unchanged guards and2/16voice prepared dispatch. Observation overhead
included, not real-time or physical timing acceptance. Keep historical passes and
failed diagnostics distinct. Preserve failed resources pending coordinated recovery;
restart requires a separate explicit recovery decision, no automatic retry/reset.
