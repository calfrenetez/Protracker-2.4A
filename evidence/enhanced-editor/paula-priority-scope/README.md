# Scoped native scheduling diagnostic — 1 October 2026

A zero-init calling-task priority scope changes only the current Task to priority5,
saves the exact signed priority, verifies task identity and restoration, and retains
its obligation on failed restoration. Acquisition failures attempt restoration;
wrong task refuses without touching another Task. This is diagnostic scheduling
context, not a production playback policy or evidence for previous default-priority
failures. No transport clock, grid, watchdog, read, output or ownership changes.

The existing finite native cadence case uses the scope only after preparation.
Both priming and cadence loops check Ctrl-C without clearing signals. Priming has
<2000 calls with Delay1 between preparation steps; cadence has <1M iterations
and actual24000-frame target, below48000 frames. Every normal/error exit restores
priority BEFORE IO or storage cleanup. Unknown restoration/guest timeout remains
HOLD; cooperative bounds cannot guarantee exit from a stuck foreign call.

Host sanitizer priority/transport/twoEClock tests PASS14.517s; priority guard was
rerun after the final header edit PASS0.595s. Tests include negative saved priority,
invalid/missing/failed acquisition, wrong-current-task refusal, failed restoration
retention and retry, inert repeat, and early exit. Native fixture cross-build with
final cancellation checks PASS. Indexedtree837f8ce15dcdab48e699d71da1a3567a095575b4,
153 compiled source hashes verified against export and Git tree, no generated.
Binary204440bytes SHA256
`ccc211578ab37801a7f4d76c58f194ffaee88315eebf133b34e69c1eef40faf5`.
Guard built, not executed. One distinct coordinated shared030 window PASS, nativeRC0, aggregate7.236s,
run1790856811741223000. Cancel/late cases0/1 pass. Startup27 calls before epoch;
priority5 cadence observed24027 frames,187 service calls/187 completions,1464 loops
at709379Hz. Maximum gap2194 ticks (~3.09ms), cost2151 ticks (~3.03ms), observed
split maxima716/1418 ticks and notification lag459 ticks (~0.65ms). All strict
256-frame entry/post watchdogs and actual late-arm refusal remained enabled.
The guest verified saved0 restored0, own identity1, scope inactive BEFORE cleanup.
This establishes only the bounded changed-context pre-boundary diagnostic. It
does not prove why earlier default-priority runs failed or qualify musical
boundaries, sustained output, frontend playback, waiting or physical timing.

Both alarms/EClock/audio/editor/master/allIO closed before27 Fast allocations
ended at zero owned bytes. Exactrun/launcher cleanup and subsequent independent
shared-locked live process/profile/bridge68030/running/all4DMAoff/pathabsence PASS.
Explicit RELEASE sent after proof; no hold, ownership or next window. Prior
FAILED default-priority evidence preserved. Next: bounded notification-driven
owner with termination/cleanup and strict actual-time service, before frontend.
Physical OFF/deferred/unprobed. No WRITE/DMA/frontend/Wait/listening qualification.
