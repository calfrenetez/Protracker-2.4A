# Native first musical boundary — FAILED

1 October2026. Final indexedtree9c5877621394842f1029f6fc97039667005c335d,
156recorded source/dependency hashes verified; pinned cross-build PASS.
PTExecPaulaBoundaryTest206088bytes SHA256
`8f44a05b67b154f69a8eed818930ee3b1942edb2fc9d4ed7afb2cbe28cd561eb`.
Earlier boundary661fef/fc2664 was superseded before execution; NOT RUN.
Other rebuilt Wait variants NOT RUN.

The native transport exposes a bounded unclocked prime API: same preparation
previously inside start, no timer opening/WRITE. Repeated readiness is stable;
start revalidates the same owner before clock binding. Host transport/native
editor sanitizer2suites PASS21.771s, including timer-free cache readiness, repeated
prime/noIO/output and inactive/null/started refusal plus existing ownership,
clock, full-song, retained DMA/abort cleanup cases. No production priority change.

One separately scoped/coordinated guarded run1790861764770375000 safely RC20
FAILED,3.015s overall.27unboundprime calls; selected32bytes were inspected as
valid/pinned/exactChip and EVERY byte0 BEFORE timer epoch or WRITE. Then ownTask
priority5 scope, actual48000-frame musical start and independent49280-frame
termination included inEVERYWait plusCtrl-C.375actualWait/375wake,376servicecalls.

Captured musical deadline482118739EClock ticks. Existing ENTRY read482119000
(+261ticks); existing CORE read482119459(+720ticks,about1.015ms at709379Hz).
Failure COREphase6/songDEADLINE11. Observationmask3 validates ENTRY/CORE only;
printed POST is retained previous data and must NOT be used as current. Private
service-epoch frames47996/outer47991 differ from song epoch; they do NOT mean
arrival before musical start. Core actual timestamp was past its musical target.
Source schedule_step refuses now>schedule_start before completion/start dispatch.
observed_output0; no successful WRITE/output or first-boundary acceptance.
Strict256watchdogs/grid/actualreads/exact musical gate remain unchanged.

ExactownTask savedpriority0/restored0/inactive BEFORE resource cleanup; original
tc_SigAlloc restored, pump/termination/independentreader/song/editor/device/allIO
closed.9Fast allocations zero owned bytes. Exactownedrun/launcher cleanup and
independent subsequent sharedlocked sole liveprocess/profile/68030bridge/running/
all4DMAoff/pathabsence PASS. ExplicitRELEASE reported to AmiConnect; no pending
Task/IO/recoveryhold/current control/reservation/next window. Uncertain closure
would park live cooperativeTask with all contexts/storage, never exit or authorize
kill/reset/retry. No parked path occurred. Physical OFF/deferred/unprobed.

This locates this run's failure at actual first-start scheduling, despite earlier
pre-startup priority5 cadence PASS. It does not prove universal timing bounds,
physical behavior or the exact origin of all prior failures. See
`docs/NATIVE_TIMING_DECISION.md` for the required implementation policy choice.
