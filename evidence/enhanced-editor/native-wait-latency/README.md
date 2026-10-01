# Isolated native timer/Wait latency — FAILED

1 October2026. Distinct indexedtree5c1856c981b7b1c6459d75fd54ff984f95866c0c,
92 recorded source hashes verified against index/export; pinned cross-build PASS.
PTExecWaitLatencyTest27832bytes SHA256
`6247242597ec212151922f23490bdad54bf7dda0dc427a4b488e9e8bdc0ac0bc`.
Native EClock/alarm host sanitizer2cases PASS0.747s.

One separately coordinated guarded run1790860305670332000 safely RC20 FAILED,
1.981s overall. Own ambientpriority0 unchanged, two private WAITECLOCK alarms
and independent actual EClock reader. No song/cache/sample/audio.device/WRITE
or MMIO. Immutable128frame grid at48kHz; fresh actual read after EVERY Exec Wait.
Independent absolute24000frame cutoff armed before loop, included in EVERY Wait
together with Ctrl-C. No signal-derived time, catchup, rebase or relaxed gate.
Bounded256-record Fast trace prints only after all timer/reader resources close.

42actual Wait returns. First41 observations have mostly128frame gaps. Final
actualframe5524-last5259=265 exceeds strict256gate; maximum target-to-observation
lag2187ticks at709379Hz. Trace shows final deadline3746276415, arm3746274816
(in advance), actual3746278602. This establishes a missed strict gap in an
isolated timer/Wait/actual-reader path without playback/core work. It does not
identify the exact scheduling/timer/read contribution or prove a universal bound.
Half-second sustained target remains FAILED. No unchanged-candidate retry.

ExactTask/original signedpriority and tc_SigAlloc verified restored; both alarms,
reader, all requests/ports/devices closed.1Fast trace allocation zero owned bytes.
Exact ownedrun/launcher cleanup plus independent subsequent locked sole live
process/profile/68030/bridge/running/all4DMAoff/pathabsence PASS. ExplicitRELEASE
reported to AmiConnect. No pending task/IO/hold/control/reservation/next window.
Uncertain cleanup parks the LIVE Task with all contexts/storage, never exits
or permits killing/reset/retry. No parked path occurred.

No playback/output/frontend/listening/physical acceptance. Physical OFF/deferred/
unprobed. Next separate changed-context diagnostic may scope ONLY its own Task
to priority5 and restore exact saved priority before cleanup; not production policy.
