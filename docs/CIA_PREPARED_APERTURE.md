# Prepared direct-clock aperture experiment

This separate diagnostic follows the failed generic CIA observation. It prepares
an immutable descriptor in the task and polls the fixed OS `ReadEClock` entry
directly in assembly. It writes one private RAM shadow word. It does not connect
a sample reader, queue, Paula, AmiGUS, audio.device or the editor's PLAY action.
The existing arbitrary-callback aperture model and its failed native evidence
remain unchanged. The first Amiberry observation **FAILED**. Physical testing remains unqualified.

The caller's original epoch, 48 kHz frame grid, first/last window, early offset128,
read cap64 and observed residency cap256 are retained. Preparation refuses if
`last - arm_at` exceeds that residency cap. For a valid monotonic entry at or
after `arm_at`, in-window pre/post samples then imply the residency bound; the
loop need not traverse a 64-bit residency policy between every read. This is
not a bound on interrupt execution under arbitrary preemption or a stuck clock
service, and does not measure the complete interrupt prologue/epilogue.

## Ownership and admission

Task preparation first uses the existing elapsed-clock arithmetic and complete
synthetic key. Immediately before arm, under the same task exclusion, binding
checks queue identity, session, generation, trigger, owner, serial, action, slot,
publication, cancellation and armed state. A stale admission produces a private
refusal descriptor, with zero reads/stores in the handler. Invalid spans,
overlap, policy, prior use or state refuse without changing the output.

The task owns descriptor, result, raw-read array and timer context until positive
vector closure. It never modifies them while armed or executing. Unrelated
interrupts have no access to that private storage. The fixed OS clock call has
no caller callback capable of changing a key or reentering the model. This
explicit contract is narrower than the existing generic callback API. It cannot
be substituted for that API or establish currentness/cancellation for a real
mutable scheduler or sample owner. Native TASK/provider bindings stay inhibited.

The handler latches nested invocation, and qualification requires a clear latch
and exactly one call per arm. No replay can infer when an asynchronous latch
occurred from its final value. Such a report is not accepted for normal diagnostic
collection. The handler never allocates, waits, releases an owner or calls C.
Its only library calls are fixed `ReadEClock` and the final task `Signal`.

## Actual samples and irreversible effects

The handler records both 32-bit clock words and the returned frequency for every
executed read, including failures. Full-width subtraction checks carry and the
high word before comparing bounded offsets, so low-word rollover is supported;
it does not turn the counter into modular 32-bit time. The loop checks rate,
regression, original first/last and a finite read count, reserving one read for
the immediate post-store sample. It never changes an epoch or tolerates a late
write. The old general-purpose callback contract is not weakened.

Only a permitted precommit sample allows the shadow store. The next actual
clock read precedes bookkeeping. Late/invalid post-read failure retains the
shadow and `commits=1`; there is no rollback or zero-effect claim. Each result
retains its complete raw read list and 1-based before/after indices. A pure C
reference replays that list after delivery, and the task checks all result fields
before continuing. This comparison is useful evidence on a target, not a HOST
proof of assembly equivalence or native timing.

Sixteen attempts use the same original frames as the previous experiment.
Twelve need exact in-window commits. The four negatives are serialized key
replacement, cancellation, deliberate late arm and disarmed admission. The
three admission refusals are task-gate tests, not asynchronous IRQ cancellation
proof. Timer/vector/task/signals and Fast-storage closure follow the existing
owned diagnostic helper. Unresolved closure retains live storage and reports
HOLD. There is no native retry or foreign timer takeover.

## Collection and current evidence

The explicit `cia-prepared-aperture-v1` profile permits at most65536 output bytes
and exactly16 complete closed observations. Its independent integer observer
checks all raw reads, decisions/effects/indices, original epoch/windows, arm
counts, fixed policy, original RC and outcome tallies. RC20 remains FAILED.
Partial output, HOLD, reentry, unknown RC or uncertain reported restoration is
failure-sticky; no automatic cleanup or replay follows. The ordinary8192-byte
RC0/fixed-marker contract and older aperture profile remain separate.

The private frozen-source HOST run passes93 groups:15 CIA/model/report groups,
70 transaction/input/upload groups and eight ordinary capture groups. Three C
fixtures run under ASan/UBSan. The new fixture includes512 paired comparisons
against the unchanged generic model under its immutable-key subset, at both
PAL/NTSC counter frequencies and44.1/48 kHz, plus boundary, rollover, stale-key,
alias, read-cap and irreversible-failure checks. Synthetic clocks are software
evidence only.

The pinned68000/soft-float/assertions/Werror build passes five ordered units,
26 offset and three size assertions,14 project dependency bodies,70 SDK bodies
and seven runtime queries. The49,636-byte HUNK has SHA256
`9cbc7aa9994d7156c2d25056669c46d7bccec12e867f6f15845a06e5c15a5744`.
This is not a complete linker-LOAD audit or aggregate stack/WCET qualification.
The default builder regression reproduces the previous47,576-byte HUNK exactly.
Initial HOST input-overflow and frozen-inventory mistakes, plus the first native
compiler's indentation-warning refusal, remain separate local evidence.

The HOST/build milestone required a new guarded exact-candidate Amiberry observation, with fresh ownership,
unchanged finite lifetimes and independent cleanup/release. No native, physical,
real-reader, scheduled output, first DMA fetch, voice-stop, device memory,
ordering, audio or listening acceptance follows from these HOST/build results.
See [compact evidence](../evidence/enhanced-editor/cia-prepared-aperture-host/result.json).


## First prepared-clock Amiberry observation — FAILED

The exact49,636-byte candidate ran once in the owned68030/no-FPU emulator with
CPU cycle_exact observed. The report completed all16 attempts and all replay
comparisons, but returned RC20: zero of12 exact timing gates and four of four
negative gates passed. There were two irreversible shadow writes. Both had late
postcommit samples and remain failures, with `commits=1` preserved.

At709379 ticks/second, normal first samples preceded the original window by
70..73 ticks. Adjacent actual readings were21..24 ticks apart, compared with
51..53 in the earlier generic experiment. The original windows remain14/15 ticks.
Ten normal attempts skipped the window without a write; two reached a precommit
sample inside it but the following actual reading was late. This is combined
code/clock-call cost, not an isolated library or interrupt-latency measurement.

Native output7656 bytes/CRC514F181E, exact custody, both guest namespace/task
absence and reported timer/vector/task/signal closure were retained. Separate
Root observation verified emulator exit and original file restoration; public
release, exact disconnected service stop and another fresh final idle check
passed. Ownership is released with no HOLD. Cleanup did not promote timing.
See [separate FAILED observation](../evidence/enhanced-editor/cia-prepared-aperture-native/result.json).

Next is a distinct shorter in-window comparison path, retaining full high-word,
rate, monotonicity, original-window and zero-before/irreversible-after semantics.
The last accepted pre-window sample is below `first`, so a full-width sample
inside `[first,last)` already exceeds it; this may permit some general polling
work to occur outside the narrow store bracket. Low-word rollover still needs
an explicit general path. This has not been implemented or qualified. No
unchanged native retry, epoch shift, phase tuning, wider window, clock/priority
change or native-provider enablement is authorized by the failed observation.
