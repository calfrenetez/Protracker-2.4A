# CIA deadline-aperture diagnostic

This is a distinct timer and RAM-commit experiment. It does not activate Paula,
AmiGUS, an audio.device request, a sample reader or a product scheduler. A linked
binary, a passing host model or a successful observation cannot grant any
scheduled-output capability flag. Native execution is not yet qualified.

The exact-scheduling decision remains unchanged. For the original caller epoch
`E`, frequency `F`, rate `R` and frame `N`, the admitted window is:

```
first = E + ceil(N * F / R)
last  = E + ceil((N + 1) * F / R)
first <= actual counter < last
```

The model calls the existing elapsed-clock arithmetic. It never replaces the
epoch, shifts the frame, subtracts measured programming lateness or widens the
window. At the existing PAL counter frequency and 48 kHz, individual windows
are only 14 or 15 counter ticks. Prior failed critical-arm measurements remain
failed evidence; this experiment is a separate question, not a retry of that
candidate or a correction to those records.

## Explicit diagnostic policy

Preparation is task work. It copies a synthetic exact key and original frame
window into private diagnostic state. The key contains queue identity, caller
session, generation, original trigger ticket/action, slot, owner and serial.
These values do not assert registration, reader adoption or actual activation.
The caller maintains immutable publication and exclusion throughout one attempt.
A native handler uses a fixed ReadEClock callback, with no allocator, editor,
cache, sample-owner traversal or queue callback.

The early aperture, maximum observed residency and read budget are explicit
experiment policy. They are not timing tolerance. The native fixture proposes
128 early ticks, 256 observed dispatcher ticks and at most 64 clock callbacks
per attempt. Public policy is bounded to 1..4096 early ticks, residency from the
early value through 4096 ticks, and 2..256 reads. The read budget includes the
postcommit sample. Two reads are sufficient if the first actual sample already
lies in the window; a pre-window sample must reserve one later postcommit read.

The single attempt samples the actual clock, checks the original key and waits
only by bounded actual clock reads. It refuses a replaced key, cancelled or
disarmed publication, expired window, clock regression/rate change/read failure,
reentry, read exhaustion or exceeded observed residency before a write. Those
refusals have `commits=0` and unchanged RAM shadow. No sample conversion or
allocation occurs in the attempt.

A permitted attempt writes one diagnostic RAM shadow word. Its immediately
preceding and following actual samples must both be in the original window to
report `COMMITTED`. If the following sample crosses `last`, fails or observes a
changed key, the result is failure with `commits=1`. The write is irreversible;
there is no rollback or claim of zero effects. A sampled clock can skip an
entire window, and ReadEClock plus validation can itself consume more than a
14/15-tick window. Such cases are recorded and fail without compensation.

Complete state, control and declared callback-context spans must be disjoint,
representable and alive. Invalid/aliased outputs refuse before writes or clock
callbacks. The callback's external OS/device storage follows its own lifetime
contract; this model does not invent its extent. State is single-attempt and
noncopyable while running. Nested calls latch reentry so the outer attempt
cannot silently succeed.

## Native experiment and records

The standalone native wrapper embeds the same host fixture once with active SDK
assertions. It allocates only its IRQ payload and 16 trace records through the
bounded Fast-memory allocator, checks `TypeOfMem`, and acquires only a free,
stopped CIA timer using the existing diagnostic ownership helper. It never
replaces an occupied interrupt vector or changes a foreign timer. The shared diagnostic ownership helper now reads the
selected timer control under exclusion and skips a running candidate before
AddICRVector or any mask operation: adding/removing even a temporary vector
can itself change the selected mask. Host tests preserve enabled running masks,
pending bits, vectors and complete count/control before-images, plus stopped
foreign-vector refusal and normal owned restoration. This prerequisite changes
only the diagnostic helper and its existing host fixture; earlier evidence is
retained unchanged. It borrows
one signal and has a separate finite 1.25-second termination request.

One original EClock epoch schedules 16 attempts at frames `(index+1)*1536`.
Twelve are normal attempts. Four intentionally test replaced key, cancellation,
late entry and disarmed publication. The explicit late-wake negative uses a
later timer deadline while retaining the original frame and its first/last
window; it must never be described as an admitted activation. At least twelve
in-window normal commits and four zero-write negatives are required, along
with clock/rate/read/residency and resource-restoration checks.

All prepared attempts are printed, including unsuccessful arm, post-arm read,
termination or callback completion. Phase and validity flags distinguish an
unavailable observation from an actual zero counter value. Records retain
original first/last, requested and observed arm times/count, arm-after sample,
actual entry/precommit/postcommit/latest read and rates, result, read count,
shadow/commit count, wake/callback counts and actual assembly dispatcher
brackets. A failed intermediate read still retains its raw returned value and
validity; it is not substituted by a predicted time or prior sample.

The assembly's first/last actual ReadEClock samples bracket dispatcher and
aperture work. The measured dispatcher span is checked against policy even
when the model refuses. ABI entry before the first sample and Signal/epilogue
after the last sample are outside that bracket. The finite read cap and observed
dispatcher-span gate therefore do not establish total hardware interrupt WCET
or a universal maximum residency under contention. Missing/regressing samples
cannot qualify a span. A success is one observation, never a worst-case proof.

The owned vector/timer must close positively while all handler data remains
alive. Uncertain close or IO/resource restoration retains storage and enters an
explicit recovery hold. There is no force close, automatic retry or automatic
release on uncertainty. An unresolved-close trace is labelled a task snapshot,
not final proof that the handler cannot subsequently execute. Only positive
closure permits Fast-storage release. Signal allocation, task identity and
priority must return exactly to their originals.

## Evidence and remaining gates

The portable host fixture covers independent arithmetic oracles at both PAL/NTSC
counter frequencies and 44.1/48 kHz, exact boundary admission, skipped windows,
already-late entry, postcommit-late failure, policy and read bounds, full key
replacement, cancellation, bad clocks, reentry and whole-image alias refusals.
The separate existing CIA ownership host fixture covers acquisition/restoration
contracts with fake resources. Host fake counters establish software behavior;
they cannot establish actual ReadEClock or interrupt latency.

The standalone builder reads literal host declarations and committed build/runtime
conventions as data. It pins the compiler, explicit GNU assembler paths, Python,
seven runtimes, full source inventory and complete C/assembly dependency closure.
It checks native ABI offsets, enabled SDK assertions, both full completion
markers once, and a HUNK header. It writes outside the frozen source and never
runs that HUNK. Any compiler failure is preserved before correction.

Native execution needs a separately reviewed exact candidate, adequate stack,
fresh target/ownership/profile/lock gates, bounded harness and independent
cleanup. Physical operation is a separate scope. No native run, CIA feasibility,
DMA timing, first fetch, stop quiescence, reader-reference transfer, audio output
or listening acceptance follows from host tests or compilation. A product
backend still needs atomic exact-target activation, edit invalidation, qualified
memory ordering, command detachment and positive reader stop/retirement proofs.

## Current host and portability status

Two focused ASan/UBSan host groups pass. An identical running-timer probe preserves
the original helper's mask corruption as an expected baseline failure and verifies
the corrected helper refuses before AddICRVector, removal or mask/control changes.
These checks use hardware stubs; actual CIA resource and register preservation
remain unqualified.

One pinned compiler/assembler build passes 16 command records, four ordered units
and 11 ABI offset checks. The 47,576-byte HUNK retains enabled SDK assertions and
both exact diagnostic markers. It is native NOT RUN. Read-only saved-record audits
do not execute it. The total interrupt span, window feasibility, stack adequacy,
physical timing, DMA, audio and listening remain separate. Earlier late native
measurements remain failed, and the exact musical gate is unchanged.
See [saved host/compiler records](../evidence/enhanced-editor/cia-aperture-diagnostic/README.md).
