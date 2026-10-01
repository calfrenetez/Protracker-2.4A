# Native Exec Wait and termination — partial success, cadence FAILED

1 October 2026. Indexed source tree61b7b9f2561c03f0fa55c6e4b6c04493fe5bdc3e;
153 compiled dependency hashes verified. Pinned cross-build PASS.
PTExecPaulaWaitTest205096bytes SHA256
`f8ae028076597e483e8282a613f011014e678a144e43b05266363f49313963d0`.
Host transport sanitizer PASS11.078s, two EClock cases PASS0.559s.

Distinct coordinated TAKE, one guarded run1790859812845142000, aggregate RC20
FAILED,5.710s overall. Case0 PASS: own termination signal before first step,
zero Wait calls. Case1 PASS: actual Exec Wait returned the separately owned
termination bit at the absolute frame192 cutoff,2Wait calls/1ordinary wake.
This proves API return/termination, not a measured duration of blocking.
Case2 FAILED:52Wait calls/52wakes, next ENTRY phase4 actual7516-last6573=943
frames exceeds the unchanged256-frame watchdog. The independent absolute
frame24000 termination request was armed and included in EVERY Wait together
with Ctrl-C; musical start was48000, so no WRITE/output was reached.
Exact source of scheduling delay is not established; failure includes enclosing
loop observation, task scheduling and next actual ENTRY read, not a standalone
measurement of time inside Wait. No retry or clock/threshold/priority relaxation.

All cases record exact Task/unchanged signed ambient priority0, all owner,
termination, diagnostic clock, device/editor resource closure and original
tc_SigAlloc restoration. Only still-owned private termination signal is cleared
after IO closure and before port deletion.27Fast allocations end at zero bytes.
Exact owned guest run/launcher cleanup and independent subsequent locked live
sole profile/process/68030/bridge/running/all4DMAoff/pathabsence PASS. AmiConnect
explicitly acknowledged RELEASE, no hold/reservation/control/next window.

The diagnostic parks cooperatively with its live Task and EVERY stack/controller/
master context on unresolved cleanup; it never returns out of a Process with
pending IO targeting its stack/Task. Such a timeout would require a recovery
hold, not permission to kill/reset/retry. No parked path occurred in this run.

Implemented owner and cancellation components remain distinct from sustained
ambient-priority cadence, musical-boundary/output/frontend, listening/endurance
and physical acceptance. Those remain open. Physical OFF/deferred/unprobed.
Next isolate timer/notification/task latency with bounded actual-clock diagnostic
observations, preserving the failure and strict production timing guards.
