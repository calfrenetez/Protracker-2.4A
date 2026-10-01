# Existing-read service timing snapshots — 1 October 2026

Native enclosing transport snapshots existing entry/core/post actual clock reads
and the prior periodic notification target. Validity mask1/2/4 prevents stale or
missing observations from being interpreted as measured intervals. Diagnostics
survive cleanup and reset on next begin/per-service. No added clock reads or
critical service prints, no change to logical elapsed time, grid or deadlines.
Snapshot stores can still perturb execution cost; this does not prove identical
performance to earlier candidates or identify host versus guest stall causes.

Five host sanitizer tests PASS29.349s after staging source: nativeTransport,
nativeEditor, twoEClock tests and PaulaSong. Tests cover injected300tick core-reader
jump/POST refusal, exact three/four existing clock reads, partial observation mask
on frequency/regression refusal, retained diagnostics, timer-free priming,
exact start and retained DMA/abort/context cleanup. Initial run failed an existing
fixture cumulative timer-call count assertion against zero; corrected to compare
per-fixture initial counts. That failure is preserved in reported provenance.
Earlier unbound-prime native host tests exported the previous Git index before
new code was staged; they did not qualify new native priming. Its README now
contains that correction. The current staged-source pass qualifies the current
host implementation only; the earlier guest binary/evidence stays unchanged.

Indexed source tree83d0af9921499872d2cf88da9ffdfa9cd3dadb70;152compiled source
hashes verified/no generated. Binary203596bytes SHA256
`f4f84b84e6373f330d6ea33367608952feface3eeb9877273af2f62d329b7b4a`.
Guard built, not executed. One separately coordinated shared030 window,
200overall/140runner/90fixture including cleanup; no lifecycle or physical.

Run1790855287109306000 aggregateFAILED/nativeRC20,6.813sec. Cancel/late cases0/1
pass; unbound startup27 calls; then21ready calls/21periodic completions. Refusal
stage11PERIODIC_ARM: target289873396, actual289873423 (27ticks late). Post frames
2812-last2747=65, within256; this is not a POST watchdog refusal. Post2812 is only
four frames before next128-grid frame2816. Last observed entry->core352ticks,
core->post610, notification lag873 at709379Hz. Max observed splits352/858/873;
max prior total cost1578ticks. Mask7 confirms all three reads. Cadence remains
FAILED, target24000frame completion unproven. No WRITE/DMA/output/Wait/frontend.

Candidate-specific normal-safe-failure checks close both alarms/EClock/audio/
editor/master/all IO before27Fast allocations end atzero owned bytes. Exactrun/
launcher cleanup and independent subsequent shared-locked running/bridge68030/
all4DMAoff/pathabsence pass. Explicit RELEASE after proof; no hold or next window.
Prior FAILED evidence remains preserved. Next: assess earlier rearm of periodic
notification using the validated immutable entry grid, while retaining a fresh
post-work watchdog and strict actual late-arm refusal; no rebase/catchup.
