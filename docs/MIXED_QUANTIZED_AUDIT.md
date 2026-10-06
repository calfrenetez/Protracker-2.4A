# Standalone strict quantized whole-song geometry audit

`mixed_quantized_audit` is an optional task-side audit of the original complete
song. It uses genuine cancellable renderer setup and the quantized boundary
normalizer. It is independent of master promotion, editor barriers, cache
preparation, readers, keys, queues, native activation and device I/O. Existing
legacy preflight and normalizer routes keep their contracts.

## Caller storage and borrowed lifetime

Query and supply three aligned, mutually disjoint buffers: audit workspace,
normalizer workspace and result storage. Zero their used bytes before begin.
Full declared capacities, the original audit owner-pointer slot and all borrowed
source/control storage remain alive and protected until exact close. The result
storage contains the genuine original normalizer owner and a separate batch.
Neither it nor the audit's original publisher may be embedded in a complete
immutable parent context. Declare all unknown complete allocator/caller contexts,
up to 30; declarations describe numeric extents and do not grant ownership.

Admission checks capacity arithmetic and the sum of all three full capacities
plus the actual queried renderer setup and sequence control sizes before writable
initialization or allocation. The audit accepts exactly two genuine renderer
control requests, with no allocation retry. Complete project/tables/PCM capacity,
slices/extensions, fixed descriptors, unused caller tails, original owner slots,
local scratch and retained/retired allocation extents are guarded. Numeric span
checks do not prove residency or permit the caller to free borrowed objects.

Calls are serialized and noncopyable. In-place source changes require the actual
revision or generation to change. Valid selected-channel cursor movement alone
is allowed. Header/tag currentness precedes former table traversal; cancellation
and close use retained numeric identity rather than walking expired source arrays.
Actual consumed child NULL and genuine positive returned identities remain
recorded even when an outer callback fault also vetoes the operation.

## Exact traversal and scope

Only whole-song, all-track options are admitted: 44,100 or 48,000 Hz, 16/24-bit
render options and supported mono 8/16-bit playback geometry format. Pattern-only,
row-range and partial-track requests refuse. Unsupported exact Paula/card
geometry refuses instead of skipping, approximating, rebasing or adding jitter.
Authoritative 8/16/24-bit masters retain their precision and save identity.

Begin PENDING and intermediate renderer setup READY do not mean the whole-song
audit is ready. `step` accepts work1..256. Genuine setup/measure/next/consume/
complete visits every renderer boundary, including initial zero-frame and empty
DONE boundaries. The real quantized normalizer begins, steps, gets and closes
before tentative origins/masks commit. Frame totals come from actual consumed
interval frames; there is no guessed frame+1 boundary. A mask is published only
on complete/current audit READY. Origins, masks and seven-field images remain
descriptive geometry; they are not validation, master ownership or ACTIVE-key
certificates.

Charged work covers bounded semantic/tick/frame/normalizer items or one local
phase. Opaque renderer measurement charges the requested tick budget, not a claim
about actual ticks. Separately finite metadata admission/capture loops remain;
this interface supplies no WCET, IRQ or wall-clock guarantee. The fixture tests
zero-frame boundaries but supplies no whole-song emit=0 case; row-range is outside
this audit's accepted scope.

## SAME sequence and cancellation

One successful `take` rewinds once and transfers the SAME genuine audited sequence;
no replacement sequence or allocation is introduced. Its guarded allocator still
borrows the audit workspace and original controls. Audit close remains PENDING
while that transferred sequence is live. Close the real sequence first: its actual
wrapped release retires the exact original allocation ledger under BUSY. Only
then may the original audit owner close and consume its slot.

Cancel marks the owner; it does not invent STOP, retirement or a device proof.
Close drains the original normalizer before renderer children. A callback failure
can accompany real child consumption, so preserve actual NULL/positive identities
and perform the later explicit numeric close instead of retrying a consumed
release. A copied owner slot refuses. Completed renderer DONE is not STOP and
retains tentative origins for the caller's subsequent independent ownership work.

## Qualification

The exact V4 HOST fixture passed once with assertions, `-Wall -Wextra -Werror`,
ASan and UBSan: 955 saved inputs, six overlays, 39 units, 28 completed quiet owned
drivers and seven full1,553-byte lines. Independent saved-only review verifies
closure/custody and all assertion groups without replay. Literal schedules,
late Paula901 refusal, full16 routing, exact centre levels, same-sequence transfer,
master-save bytes, aliases/budgets/cancellation and callback ownership are covered.
Original V1 SOURCE, V2 compile and V3 runtime failures remain separate; V3's
unrecorded return/tuple remains unknown.

Native compiler/runtime, Amiberry, real A1200, memory placement, aggregate stack,
launcher admission, device capacity/order/completion/voice stop, exact live output
timing, audio and listening remain unqualified by this candidate. It supplies no
whole-song master-owning producer or composite editor PLAY integration. See the
[saved HOST packet](../evidence/enhanced-editor/mixed-quantized-audit-host/README.md).

## Separate portable compiler qualification

The preceding HOST qualification records the earlier stage. A separate first
compiler/link attempt now passes at committed admission
`641c281c43054a63612b001897cb6676e2a5e04d`. Its copied source pool is the exact
955 inputs from the accepted saved HOST run, reconstructed from 950 archived
files, six overlays and the generated font. The genuine audit C remains included
by the fixture, with 39 ordered translation units. Assertion-enabled C99,
`-m68000 -msoft-float -mcrt=nix20 -Os -Wall -Wextra -Werror -UNDEBUG` compiled
and linked a 232,864-byte HUNK product, SHA256
`58b3b9eb2f02ffc1d6c081f8773dbb8df438111addf5bab3a74da18dbe63e840`.

Independent saved-only review checks all 1,081 retained inner files, the actual
124-path raw `-M` union (92 copied source files and 32 selected SDK files),
52 compiler drivers, eight inner Git readbacks and ten root drivers with all
140 raw streams. Recorded drivers returned zero with empty stderr, were reaped
and left their owned groups quiet. Complete current source controls, protected
16 paths, empty staging and before/after saved Git admission match. Recorded
inner/root times are 9.711/10.730 seconds within their 600/900-second envelopes.

The product is NEVER_EXECUTED. HUNK header and all seven complete marker strings
are static byte checks, not native assertion results. This compiler milestone
supplies no Amiberry or real-A1200 runtime, memory placement, stack/IRQ/launcher,
device capacity/order/completion/voice-stop, live schedule, audio or listening
acceptance. Whole-song master ownership and composite editor PLAY remain
unfinished. Original SOURCE and HOST failure histories remain immutable; no
failed attempt was replayed. See the separate
[compiler-only packet](../evidence/enhanced-editor/mixed-quantized-audit-compiler/README.md).
