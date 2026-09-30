# Mixed running-service cost qualification

1 October 2026. Production unchanged. Host195 ASan/UBSan scenarios PASS7.985s:
189 previous plus6 complete2/16voice cases65..66 at8/16/24, selected classic Paula
L/R/R/L4voice geometry and12AmiGUS channels. Sample2 retriggers channel7 while a
global tempo change affects both routes. Two positive intervals reach terminal
DONE at1000+the preflight frame bound. Preparation performs no Fast allocation or
output, at most one Chip allocation OR256-byte upload (128 fake bus writes) per
call. After64 preparation calls at the current logical frame,128-frame services
and exact trigger/control/end commits perform no allocation/upload. Commit order
is wave trigger then ascending Paula/AmiGUS controls; final end has no new output.
Sources/precision and all reader/master/cache ownership remain preserved.

Selected isolated build162indexed source hashes/zero generated/3verified binaries,
tree e3d33fa8e1588eec6c4d78b9226fc9b95cdc1a23. Native PTExecMixedOwnerTest231864bytes,
SHA256407d1cee5018969a397e37fb4992fc1cbf4070d815d0a110f86852dbd0449107.
Native ONLY6 new complete-song cost cases, not all195 host scenarios. ONE run
1790810267832235000 PASS12.018s overall.66actualFastallocations/zero owned bytes,
actualChip release assertions and all pins/readers/leases/private counter/device/
request closed. Fresh AmiConnect/peer coordination and standard shared locked
live guards, exact cleanup and independent later locked running68030/bridge/
all4DMAoff/run+launcherabsence PASS. Explicit RELEASE. Original200s overall/
140runner/90fixture/reservation<=240s; no retry/reset/lifecycle/physical operations.

858 real EClock observations (709379Hz): per voice count384prepare,39live services,
6boundaries.2voice prepare0.961..32.237ms, service4.197..25.270ms, boundary
4.807..25.205ms.16voice prepare0.960..62.591ms, service17.947..38.417ms, boundary
9.226..34.946ms. All observations/outliers retained. Private unsubmittedUNIT_ECLOCK
only; no alarms, actual output/MMIO/DMA/audio or physical acceptance. Preparation
is deliberately fully primed at unchanged injected logical time; no claim it fits
real frame cadence.128frames at48kHz represents2.667ms, which is below even the
minimum observed live cost. Thus successful logical scheduling is NOT demonstrated
actual-time playback feasibility. EClock instrumentation/fake callback and emulator
scheduling costs prevent physical extrapolation.

Inspection identifies frame-by-frame pt_voice_advance in both live consume and
lookahead, rather than software sample mixing. Next investigate exact bulk advance
while preserving one-shot, loop/pingpong, segment/repeat source and overflow behavior;
validate state equivalence before remeasurement. No timing policy change.
