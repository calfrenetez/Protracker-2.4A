# Unbound startup prime — 1 October 2026

Core/editor prime advances one startup transition without a clock, to ready NEXT
or ZERO. Public interval/numerical schedule APIs refuse while priming. Repeated
ready prime is inert. Clocked begin accepts only ready priming, checks frame
extent overflow before reading, samples the actual counter once, revalidates
ownership after the foreign reader, and preserves the prepared batch while
establishing the immutable epoch/start delay. No live epoch rebasing or catchup.
Native start returns PREPARING for each bounded prime step, before timer opens.
Caller must continue start until readiness; borrowed owner/context stays alive.
This deliberately changes startup semantics; previous timed-preparation FAILED
candidates remain preserved and are not cleared under identical conditions.

Host corrected PaulaSong/nativeTransport sanitizer PASS17.613s: full8/16/24 song
progress, ready inertness, invalid phase/no clock read, unfinished begin refusal,
stale prime/reader, primed extent overflow, retained unbound cancellation before
and after readiness, timer-free prime, exact start and retained DMA/timer cleanup.
Editor/nativeEditor suites passed in earlier38.848s four-suite invocation, which
had a new already-primed expected-result assertion failure. That test expectation
was fixed; no aggregate four-suite pass is claimed.

Indexed tree de7208c7c1d03b23dfe17d3417756b1976915795,152compiled source hashes
verified/no generated source. Binary202740 bytes SHA256
`ea127bc6564ede6246c51cc3dda410881ef0ae2f6acd28d78b9dc4d9db287bd2`.
Guard built, not executed. One separately coordinated shared030 window with
200sec overall/140runner/90fixture including cleanup; no lifecycle or physical.
Selective32-byte Chip preparation permitted; no WRITE/DMA/output/ExecWait/frontend.

Run1790854272771419000 aggregate FAILED/nativeRC20,6.807sec. Cancel/late cases0/1
pass. Initial unbound startup27 calls checks no timer/WRITE before readiness.
Periodic ready service reaches43calls/43completions at5513observed frames, then
POST watchdog refuses6594-last5518=1076>256. Readiness1,prime0,ready43. Maximum
prior ready-call cost2116ticks (~2.983ms), lastobserved15973ticks (~22.517ms),
frequency709379Hz. This isolates a ready-service overrun but does not establish
its cause, stable cadence, target24000frame completion or output acceptance.

Normal-safe-failure path confirms both alarms/EClock/audio/editor/master closure
before27Fast allocations end withzero owned bytes. Exactrun/launcher cleanup and
independent subsequent shared-locked running/bridge68030/all4DMAoff/pathabsence
pass. Explicit RELEASE sent after proof. Resource closure is separate from FAILED
cadence. Previous results, strict thresholds and immutable epochs preserved.
