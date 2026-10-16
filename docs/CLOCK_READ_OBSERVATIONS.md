# Adjacent clock observations

The prepared CIA RAM experiments failed their original pre/post clock windows.
Their combined brackets do not isolate library clock-reading cost, and do not
locate the actual RAM write within a failed bracket. Exact scheduling remains
required. No production backend, epoch, phase, rate, priority or window changes.

This separate diagnostic opens its own timer.device UNIT_ECLOCK request and
records sixteen pairs of direct ReadEClock calls from the owning task. Each pair
runs under Disable/Enable. Only the first returned-rate store and the address of
the second output separate the calls in the assembly. Full high/low words and
both returned frequencies survive. The library sampling instants are observed;
call entry and return times are not. There is no CIA vector/timer acquisition,
shadow commit, hardware register write, audio or scheduled activation.

One bounded Fast allocation holds all sixteen payloads. Seven offset and three
size assertions check the assembly layout. Task identity, priority, interrupt
and task nesting, signals, timer request/port and zero owned pool bytes are
checked at closure. A finite pair count cannot prove trusted OS calls return
within a hard bound or establish an interrupt WCET/stack limit.

The shared C analyser checks complete counters, rate mismatch and regression;
it never treats backwards time as modular elapsed. Invalid or overlapping output
extents remain unchanged. The host fixture includes 1,024 full-width arithmetic
comparisons, rollover, equal timestamps, rates and invalid output cases. A
separate Python saved-report replay checks every pair and closure tally.

The explicit `clock-pair-v1` collection profile accepts only sixteen complete
closed records. RC0 is `OBSERVATION_COMPLETE`, not a scheduling PASS. A complete
invalid-clock report retains RC20/`FAILED_CLOCK_OBSERVATION`. Partial reports,
uncertain closure and other return codes keep transaction failure sticky. The
ordinary fixed-output contract and both older CIA profiles remain separate.

Observed min/max describe only these finite sampling intervals, including the
minimal intervening instructions. They are neither an isolated call duration,
universal minimum, upper bound nor physical timing qualification. Do not use
them to retune a phase or claim a timestamped output command exists.

Current host and pinned three-unit 68k compile evidence is recorded in
[the compact result](../evidence/enhanced-editor/clock-pair-observation/host-build.json).
The exact emulator and physical observations are recorded below. Actual
output timing and human listening remain untested by these checks.


## First exact-candidate Amiberry observation — 16 October 2026

Candidate 19,952 bytes, SHA256
`919196b89b92de580cf24ff32cd63ca82a616d1d36919bff79831cd39baf162c`
(CRC32 `CEE66A58`) ran once in a fresh guarded 68030/FPU0 profile. All sixteen
pairs completed with returned frequency 709379 and unchanged task priority0.
Their actual sampling deltas were 19..21 ticks (fourteen20, one19, one21).
These observations already exceed the earlier 48kHz frame windows of14/15
ticks in this profile. Further small reductions to intervening RAM checks do
not address that observed measurement baseline. This is a finite observation,
not a universal lower bound or a proof of the time of any actual output write.

Native analysis and independent saved-output replay agree. RC0 is retained as
OBSERVATION_COMPLETE. Reported task/nesting/signals/clock/Fast closure, exact
file custody, separate task/both-namespace absence, own emulator exit, original
launcher/config restoration, public release, exact own service shutdown and a
separate fresh final idle check passed. The release ended at fence1675 with83
archives and no HOLD. Those are recorded release facts, not future availability.
See [compact native evidence](../evidence/enhanced-editor/clock-pair-observation/emulator.json).

Safari access was checked after release: the Mac remained locked and automatic
unlock failed. No physical session, service, control or probe was acquired.
Current physical timing remains untested. Continue authentic device-activation
contract work and independently useful memory/save integration; do not retune
the musical grid, repeat an unchanged failed CIA candidate or enable a native
provider from these observations.

## Updated transport and first A1200 observation —16 October2026

The unchanged19,952-byte candidate ran once in a fresh Amiberry transaction
using the bounded directory completion observer and approved256-record guard.
All16 pairs completed: thirteen20-tick, two19-tick and one22-tick intervals.
RC0 remains OBSERVATION_COMPLETE. The updated path
passed67 runner and27 service host tests, including complete and failed-clock
profiles. Exact custody, independent cleanup/release, own service stop and a
separate final idle observation passed. Initial host-only sandbox process-access
and offloaded-header refusals remain preserved before any service or target
operation; restoring the header retained its original hash.

After that exact qualification and fresh physical admission, the candidate ran
once on the owner's68030 A1200, Exec47.13/PAL. All16 valid pairs completed with
frequency709379 and unchanged task priority0: the first interval was17 ticks and
the remaining15 were15 ticks. Both returned clock words/rates and all reported
task, nesting, signal, timer and Fast-memory closure checks survive independent
saved-output replay. The physical path passed43 runner and27 service host tests;
an initial relocated offline helper import failure is preserved separately from
the corrected no-effect construction check.

One input and eight execution files were retained for custody, then their exact
RAM namespaces and task absence were observed independently. Installed host
configuration stayed unchanged. Public release, exact own-service shutdown and
a separate final idle check passed: fence1877,131 retained archives, no HOLD.
Safari was Authenticated Connected View Only before and after; no browser
control or input was acquired. Release facts are historical and cannot clear a
future operation.

The real-machine intervals differ from the emulator's finite19..22-tick sample.
Neither set isolates call duration or establishes a lower bound, WCET, IRQ
latency or the time of an output write. These observations do not qualify the
earlier14/15-tick output brackets, scheduled activation or listening. Preserve
the exact schedule and native provider inhibition; no failed CIA experiment was
replayed and no timing window, epoch, rate or priority was changed. See
[updated emulator evidence](../evidence/enhanced-editor/clock-pair-observation/transport-emulator.json)
and [physical observation evidence](../evidence/enhanced-editor/clock-pair-observation/physical.json).
