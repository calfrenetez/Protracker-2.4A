# Genuine sampler binding for split reader/command ownership

`sampler_paula_readers.c/.h` is a separate opt-in task-side binding for
`scheduled_readers`. Existing future, lineage-v1, flags7 and all-reader retirement
APIs are unchanged. This binding uses real immutable sampler versions and a
separate optional selective signed8 Chip representation cache; it does not install
or qualify a native activation backend.

The pool has independent bounded command and persistent-reader handle capacities
(1..8 each), a control-allocation budget and a Chip-cache budget. The legacy
`pt_paula_readers_open` still validates and allocates synchronously. Optional
initial setup now uses the cancellable workspace described below. Each command preparation step
performs one transition/action, copies at most 4096 master bytes, or converts/copies
at most 256 Chip bytes. A TRIGGER retains one genuine master pin and one exact cache
lease. It preserves authoritative 8/16/24-bit mono/stereo data and low bits; stereo
requires an explicit selected channel. No implicit rate conversion or downmix is
added. Cache geometry and format follow the existing Paula representation contract.

A command holder owns only small command/request/batch metadata. Each TRIGGER has
an independent persistent-reader holder containing its genuine pin/cache lease,
original numeric source/route and geometry, monotone holder token and immutable
original trigger/action identity. Its full sample/cache capacities are registered
as reader-resource spans; mutable reader and command holder extents are separate.
Every held version/backing, marker and cache allocation stays alive until its exact
reader-domain release callback. The pool keeps caller-owned original source spans
for output protection even after promotion or invalidation.

CONTROL/STOP requests contain only a by-value full original key and numeric
track/sample/channel. The key must come from the adapter's positive reader-key
getter, which forwards the actual caller output to the core guard. It records a
key only after actual ADOPTED+ACTIVE qualification. Begin, preparation and enqueue
first resolve that exact cached full key to a registered LIVE reader, then recheck
the core's actual original-key permission and exact current version/lease. Forged
session, serial, owner, trigger, action, slot or generation refuses before holder
callbacks. There is no raw predecessor pointer, source descriptor, master pin or
cache lease in a CONTROL/STOP preparation or READY holder. A predecessor may retire,
be replaced, change version or close while a command is prepared; enqueue re-resolves
the copied key and refuses safely. Matching cache address/serial alone never grants
original-key permission. The core's monotone same-slot replacement and STOP rules
remain authoritative.

| Proof or operation | Small command holder | Persistent reader and master/cache |
| --- | --- | --- |
| Prepare/cancel before transfer | Local holder can close | New untransferred trigger resources cancel; controls own none |
| Successful enqueue | Transfers independent command control | Each new trigger transfers its independent genuine reader control |
| Actual command DETACHED | Retires command; explicit close permits slot reuse | Retains all reader storage |
| Actual reader RETIRED while commands refer | Command ownership unchanged | Core retains the reader owner until last command detach |
| Reader RETIRED with zero referring commands | Command ownership unchanged | Exactly-once release unpins the cache and immutable master |
| Unknown/malformed/partial ownership report | Retains possible ownership | Retains possible ownership; independently exact domain proof may release with invalid terminal classification |

After a trigger transfers, retrieve its independent reader handle before closing
the detached command. Its original key and frame persist across command slot reuse.
The command's reader-handle getter verifies live registration plus token before
reading an origin pointer, so a recycled reader address cannot revive an old handle.
Retired reader handles remain registered until explicit close; LIVE close refuses.
Pool close refuses every registered handle and never force-unpins or automatically
cancels submitted work. Retired resources can still close after source/generation
invalidation. A forbidden callback reentry during positive retirement defers resource
release to explicit reader close; it never discards the confirmed retirement.

All adapter-known outputs and input declarations protect full master capacities,
original project metadata, sampler versions/jobs, cache storage, pool and handle
controls before mutation. Guard scans metadata only. Invalid aliases leave output
and state unchanged. Caller must additionally keep arguments disjoint from opaque
queue/backend/allocator/Chip callback contexts whose full extent the adapter cannot
know. Public actual reader-key output goes through both adapter and core guards.
Contexts and original borrowed storage stay alive through pool close. Calls are
serialized task work; callbacks forbid reentry or editing borrowed inputs. Only
designated release callbacks release their corresponding independently retired
holder's resources. No owner is copied/cast from a legacy exclusive owner.

The shared host fixture uses genuine sampler promotion/cache operations with host
allocator injections. Its bounded backend model represents positive clock/key/
adoption/detachment/retirement receipts in serialized task work; it is not a native
IRQ implementation. Six precision/channel cases retain the full original masters,
promoted version-header before-images and exact single cache pin through 20 CONTROLs with two
command slots, then separate STOP/reader retirement and command detachment. Further
cases cover 52 preparation cancellation points, allocation/budget pressure, cancelled
controls without extra pins, retirement before/after READY, replacement retirement
before stale admission, source-version change, real reader-address reuse with fresh
token, forged keys, full-capacity/header/backend output aliases, uncertainty/wrong
retirement context/invalid terminal classification, and finite callback reentry.
Each fixture ends with zero owned allocations and budgets. Host tests do not prove
physical Fast/Chip classification, installed backend reference transfer, hardware
voice-stop, actual scheduling, DMA/IRQ/audio or human listening acceptance.

## Optional cancellable initial setup

Zero-initialize a caller-owned `pt_paula_readers_preparation`, then call
`prepare_begin`, `prepare_step`, and `prepare_transfer` with the same current
project revision. Begin inspects metadata and returns OK for initialization;
it does not validate the semantic PCM or publish a pool. Step accepts a work
budget from 1 through 4096 and returns PENDING until the entire current project
passes. Each call advances at most that many validator items, with additional
fixed metadata checks. Pending and completed validation own no pool, master pin,
Chip allocation, cache lease or backend resource. Call `prepare_cancel` at any
point between calls to clear the workspace without reading former source arrays.

Only transfer of a complete, current workspace performs the fixed pool allocation
and initializes the private bridge. It avoids repeating either synchronous PCM
scan. Allocation failure leaves completed preparation retryable; successful
transfer consumes the workspace. A fixed-header or sampler identity change
refuses before source traversal, including after the allocator callback. Callback
reentry latches failure and releases any returned unpublished pool. Ordinary
refusals leave the caller's pool output unchanged.

Keep the project, full-capacity masters, tables, sampler, original allocator and
configuration objects, and opaque callback contexts alive through transfer or
cancellation. Source values and metadata must remain immutable within every call,
including callbacks. Between calls every edit must change the caller revision or
sampler generation; a valid channel-selection cursor is exempt. Scalar revision
arguments cannot detect an arbitrary in-place edit during a callback. Workspace,
outputs and callback contexts must remain disjoint from borrowed source and control
storage. The guards include unused master capacity. There is no public unchecked
bind or reusable validation certificate.

This is opt-in task-side preparation, not editor PLAY integration. Fixed pool
allocation, metadata checks and final publication remain synchronous; the work
budget is not a native wall-clock or interrupt latency guarantee. Existing
synchronous APIs, command/readers ownership and the original musical grid remain
unchanged.

## Optional renderer-boundary lowering

The optional boundary adapter lowers one borrowed, immutable renderer plan at its caller-supplied absolute musical frame. It copies normalized numeric ranges, pitch and gain into the existing bounded command holder. Supported Paula triggers are mono 8/16/24-bit one-shots with exact initial phase and even bounds; selective signed8 playback caches remain derived from authoritative masters.

The renderer may emit a TRIGGER followed by the same voice's final CONTROL. Lowering combines that pair into one trigger using the final pitch and volume, retaining the original source/range/phase. Other repeated or unsupported Paula operations refuse the whole boundary. Known other-route actions are ignored, while unknown kinds and invalid channels refuse even outside Paula tracks; ignoring another route grants no output permission for it.

A continuing CONTROL or STOP requires the complete original key positively acquired from the genuine reader after actual adoption. Its source sample and PCM channel come from the registered original reader, independently of the renderer's current instrument. Copied keys are rechecked during preparation and transfer. Submit acceptance, a matching cache address, a reader view and command detachment do not supply ACTIVE permission.

Use the existing cancellable preparation steps and explicit command/reader close operations. The specialized enqueue resolves held cache bytes at transfer and binds the copied normalized geometry. The general enqueue enforces the same binding for lowered commands. Inputs and returned storage protect full known source/control capacities, including caller plan/capabilities/key storage and private staging during allocator callbacks. Opaque allocator/backend contexts still obey the caller-disjoint lifetime contract.

Lowering leaves sequence/lookahead consumption and commitment to the caller. An empty terminal renderer plan creates no command; explicit STOP and independently confirmed reader retirement remain separate. This API does not provide whole-song transport, activation, interrupts, DMA, device completion, timing or listening qualification.

The copied lowering fields enlarge each opaque command allocation. The existing
control budget charges the actual allocation size; it remains bounded and can
refuse capacity. No new master or Chip allocation is added to CONTROL/STOP.

## Current validation status

The frozen shared fixture passes one ASan/UBSan host group with 21 full dependency
checks and one actual execution. Its cache-byte comparison uses the existing public
playback packer as the parity oracle. Original master PCM and promoted version headers
are checked before and after ownership transitions; the fixture does not snapshot
every owned version PCM byte or explicitly assert adopted-table/pin-job aliases.
The source guards cover these known spans, with those test limits kept explicit.

One pinned 68k portability build passes 32 command records and produces a
157,632-byte HUNK with active SDK assertions. It remains native NOT RUN. No installed
backend, actual device-domain transfer, Fast/Chip memory-class qualification,
IRQ/DMA, timing, audio or listening acceptance follows.
See [saved host/compiler records](../evidence/enhanced-editor/sampler-paula-readers/README.md).

The separate 4 October initial-setup qualification passes the final startup
ASan/UBSan fixture, the unchanged genuine-reader ownership fixture, and three
affected Paula/validator/scheduled-reader regression groups. The final setup
fixture tests 18 format/work combinations, poisoned semantic storage at begin
and completed transfer, all observable cancellation phases, late invalid values,
stale/freed former tables, full-capacity aliases, allocation refusal/retry,
callback identity changes/reentry, empty projects and 20 zero-owner lifecycles.
The host workspace is 2384 bytes. Earlier preparation/parser refusals and the
passing version before the empty-project guard are retained separately.

The earlier startup qualification builds produce `PTPaulaReadersTest` (168816 bytes)
and `PTPaulaReadersStartupTest` (155792 bytes). Both preserve enabled assertions,
68000/software-float flags and unchanged source closures; each remains native
NOT RUN. These are ordinary injected-allocation fixtures, not Exec or hardware
memory-placement tests. They supersede the older portability products for this
source without changing their saved historical evidence.

One same-workload Mac CPU observation on an 8 MiB stereo24 master changed the
largest setup call from 20.276 ms to 0.085 ms, across 1 versus 515 calls. Total
observed setup CPU was 20.276 versus 15.210 ms; requested heap payload stayed
28472 bytes, with one pool allocation, no Chip calls and zero final owned bytes.
This is not 030 elapsed latency, a worst-case bound, RSS or physical acceptance.
See [the scoped startup evidence](../evidence/enhanced-editor/sampler-paula-readers-startup/README.md).

## Renderer-boundary qualification — 4 October 2026

Seven unique ASan/UBSan host groups pass once each for the frozen source: the
new boundary fixture, genuine readers, initial preparation, Paula preflight,
render sequence, render plan and scheduled readers. The boundary fixture drives
actual transferred preflight/sequence/lookahead at 48 kHz through mono8/16/24
songs with four simultaneous Paula slots, tick-zero pitch/volume folding, at least
80 continuing controls, an unselected global-tempo track, sample replacement
and delayed command detachment/reader retirement. It checks full initial master
and retained version spans, cache parity, copied input lifetime, cancellation,
geometry substitution, malformed plans and allocator/output aliases. Empty DONE
and separately explicit STOP retain their distinct meanings. The 44.1-kHz direct
boundary path, every forged field/cancellation point and arbitrary callback
padding mutation are not independently exhausted by this fixture.

The three-group proof records six successful compile/run commands and 73 full
dependency queries; four affected regression groups record nine successful
commands and 37 queries, including intentional retained counting sources. Each
proof preserves actual products, complete dependencies and unchanged source
inventories. Independent product/proof review is clear; that reviewer authored
the boundary fixture and did not independently execute it.

Current pinned compiler-only products are `PTPaulaReadersTest` (172684 bytes),
`PTPaulaReadersStartupTest` (159792 bytes), and `PTPaulaReadersBoundaryTest`
(215772 bytes). The first two record 33 successful commands/22 ordered dependency
queries each; boundary records 40/29. All retain active SDK assertions,
68000/software-float flags, seven unchanged runtime inputs and HUNK output.
These products bind this current source, superseding the previous startup
products for subsequent qualification while preserving their original records.
All three remain native NOT RUN. They use ordinary injected allocation; no
Exec memory-placement, native stack adequacy, backend transfer, DMA/IRQ, exact
hardware scheduling, physical device, audio or human listening acceptance follows.
No target window, lock, control or queued target action was acquired for this slice.
See [saved boundary proof](../evidence/enhanced-editor/sampler-paula-readers-boundary/README.md).

## Optional whole-song producer over this binding — 5 October 2026

`paula_readers_song.c/.h` adds an optional owner around this genuine boundary binding.
It drives actual opaque initial preparation, complete Paula audit, the same
transferred sequence and lookahead before explicit publication. Current nonempty
master versions must exist before begin; no PCM promotion occurs while the sequence
borrows them. Its private fresh pool/queue use exactly 2 command and 8 reader records,
while this binding's generic 1..8 APIs and historical qualification retain their scope.

The owner preserves each original absolute boundary through WAIT_ACTIVE and pressure.
Only a positively observed exact prospective original authorizes CONTROL/STOP; a
future replacement can reject old-key admission before backend activation. Explicit
service uses actual retained tickets/actions and independently drains exact command
and reader proofs, including after sticky failure. DONE, explicit terminal STOP,
detachment and retirement keep separate meanings. Cancel/close do not implicitly
poll, emit musical STOP or force release. Close needs a real live typed owner slot;
poisoned bytes are not an owner handle.

Frozen production-v2/fixture-v5 passes three ASan/UBSan host groups once each,
six compile/run commands/70 full dependency queries, 82 canonical/107 SDK inputs,
three saved products and no generated inputs. One pinned assertion-enabled
68000/software-float build passes 40 commands / 29 full queries,79 canonical/32 SDK
inputs and seven exact runtimes; both markers survive. Its 263416-byte HUNK
`PTPaulaReadersSongTest` is never executed. Independent saved host/compiler-proof
reviews pass within their scope. The host fixture exercises whole-song parity,
2/8 pressure, selected uncertainty/
malformed-domain drains, original overflow/LATE and callback closure cases. All
frozen failed v1-v4 versions remain preserved; unused PCM-padding close and exhaustive
fault/alias/budget matrices remain uncovered. Its serialized model is software
ownership evidence only. Constructor metadata structurally exceeds a nominal
65536-byte 32-bit stack; no native stack adequacy, backend activation, physical memory class,
DMA/IRQ/timing/audio or listening acceptance follows. Existing editor PLAY and native
backend paths are untouched. See [producer contract](PAULA_READERS_SONG.md) and
[saved evidence](../evidence/enhanced-editor/paula-readers-song/README.md).


## Exec-backed memory fixture prepared — 16 October 2026

The genuine reader/command lifecycle fixture now has a separate native allocator
wrapper. Promoted versions and control/snapshot allocations use the bounded
Fast-only pool. Cache allocations use the existing Chip-only allocator and
reserve. Both ends of registered allocation extents are classified, including
reused control storage, and final pool/cache accounting must be zero. The initial
borrowed source lives in the diagnostic's task stack; its memory class is not
asserted. Backend clocks, adoption and retirement remain serialized software
models. No provider/IRQ/DMA/audio or scheduling path is enabled.

Eight ASan/UBSan host groups pass, including the actual wrapper with injected
Exec memory classifications, unchanged ordinary reader/preparation/scheduled
ownership work, the old cache wrapper, and renderer-boundary coverage. The build
recipe now includes the current host declaration's paula_render_voice unit and
selects the Exec wrapper explicitly; the ordinary portable recipe also builds.
The native product is194240 bytes/SHA256
`0870a3eda309b0f551d9d81520e9bc64dbaaec1f04f996eae6877c88f2e753e8`
(CRC32 `9FF78A26`), with24 units/72 project dependencies/49 SDK bodies/seven
runtime queries and enabled assertions. The later exact-candidate Amiberry attempt
was incomplete, as recorded below. Compiler local-frame reports reach 252,000
bytes; the proposed 1 MiB task Stack is a
conservative diagnostic choice, not aggregate/library stack qualification.

A current regression also exposed a stale boundary-fixture assumption. The
unchanged baseline compared an entire opaque version header across registration
of a second version. Its intrusive previous link legitimately changed. The
original failed run and a distinct layout-authenticated diagnostic are retained.
A test-only TU now compiles genuine sampler.c exactly once, avoiding a copied
private layout. It compares all header bytes except the reference count and two
intrusive links, requires positive references, and rejects a byte fault at each
of196 other header positions in the host layout. The actual intrinsic list is
checked separately. Full retained PCM capacities remain byte-exact; the whole
header is not omitted. No application accessor or sampler implementation changed.
The corrected boundary fixture passes. Historical boundary products retain their
original source and must not be relabeled with this corrected fixture.

See [compact host/build evidence](../evidence/enhanced-editor/readers-exec-memory/host-build.json).
The exact candidate was attempted once in Amiberry. Its execution stage reached
the original 90-second limit without confirmed completion, native return code,
output custody or observed Stack. The requested 1 MiB Stack was not independently
observed. No native memory assertion is qualified and the cause is unresolved.
No further guest exchange was issued after HOLD.

Routine recovery verified both owned process identities, stopped each with one
SIGTERM, restored saved launcher files and removed exact host staging. Separate
observation, public release and fresh post-release absence passed; the original
three HOLDs were archived, with recorded EMPTY state at fence 1681 and 84 archives. Emulator exit ends
its RAM lifetime; it does not prove native allocator cleanup. A new operation
requires fresh availability checks. See [incomplete attempt and recovery](../evidence/enhanced-editor/readers-exec-memory/emulator.json).

The next candidate needs useful bounded progress diagnostics. This failed
transaction will not be replayed. Physical promotion remains unavailable for
these bytes, independently of unresolved live output timing.

## Separate closed-file trace candidate — 16 October 2026

The optional `--exec-memory --trace-memory` build preserves the full six-format
reader workload and all assertions. It records 92 phase lines around preparation,
the 20 continuing controls, STOP, retirement and each negative-case group. The
four original success lines remain, giving 96 exact stdout lines. Each phase
also replaces and closes the separate run-owned progress file before flushing
stdout. This reuses the existing bounded collector; it never reads active stdout
or changes the original completion deadline. The final sidecar must match the
finished phase in addition to complete stdout, RC0 and custody.

The default reader fixtures emit no trace. The new trace candidate passes the
full allocator wrapper under the injected host Exec model, plus existing
ownership regressions and transaction fault checks: 45 tests in ten modules.
Even all trace lines without the completion flag still fail at 90 seconds, and
RC20 still refuses success. The pinned 24-unit build passes with active assertions.
An initial compiler preflight incorrectly required dynamic formatted lines to
exist as complete literals; its refusal is preserved. The corrected recipe
retains all four original literal-marker checks and separately pins the dynamic
contract that was executed byte-exact on the host.

`PTExecReadersTraceTest` is 198,256 bytes, SHA256
`0abcba58d5e43cddeadabf99f4545034ad2cfd30cd909564ecfdc4274c285204`.
Its subsequent Amiberry attempt was incomplete. Four saved closed-sidecar
observations reached the 16-bit stereo controls, a retirement race, the
cancellation matrix and then alias checks at about 85 seconds. The original
90-second execution limit expired without confirmed RC, complete output,
observed Stack or zero-owned-memory evidence. These are progress observations,
not individual case passes or a per-function performance measurement.
Routine owned recovery, independent observation, public release and a separate
fresh idle check passed. The three HOLDs remain archived; recorded EMPTY state
is fence 1687 with 85 archives. The failed transaction is not replayed.

Tracing adds diagnostic I/O; no performance or timing result follows.
Library stdio allocations remain outside explicit pool
tracking. The backend is still a serialized software model. The earlier
untraced timeout is unchanged and cannot qualify this distinct candidate.
See [trace host/build evidence](../evidence/enhanced-editor/readers-exec-memory/trace-host-build.json).
The [incomplete native attempt and recovery](../evidence/enhanced-editor/readers-exec-memory/trace-emulator.json)
are separate. The next native candidates split lifecycle and negative-matrix
work into complete components within the unchanged time bounds; the full host
regression and original assertions remain required.

## Native workload components — 16 October 2026

`--reader-component lifecycle` runs all six 8/16/24-bit mono/stereo lifecycles,
including 20 continuing controls, explicit STOP, independent retirement and
byte-exact master/cache checks. `--reader-component faults` runs the complete
existing negative matrix, including all 53 cancellation phases. Each uses the
same Exec wrapper and requires zero owned Fast/Chip bytes at completion. Their
success markers explicitly name the excluded group. Full regression is still
the default; neither component alone is the full suite.

Both complete components pass ASan/UBSan with injected host memory classes.
Their phase bodies concatenate to the original full workload in the same order.
All 46 tests across ten modules pass, and both assertion-enabled 24-unit native
builds pass. The lifecycle product is 183,388 bytes; the fault product is 194,844
bytes. Both subsequently passed separately in Amiberry. They retain the existing trace/RC/custody
contracts and original time limits. No production implementation, scheduling
tolerance or native provider flag changed.

The exact lifecycle component subsequently passed once in Amiberry: RC0,
observed 1 MiB Stack, all 80 lines, Fast/Chip extent assertions and zero explicit
owned bytes. All six precision/channel cases completed their controls, STOP and
independent modeled retirement. Exact input/execution custody, task and namespace
absence, original-file restoration, independent release, owned-service shutdown
and a separate fresh idle check passed. Recorded EMPTY state is fence 1691 with
86 archives; fresh checks remain required before any later operation.

This qualifies the lifecycle's explicit allocations on that guest. The combined
full native workload, real backend activation, physical memory pressure and
musical timing/listening remain separate. See [lifecycle native
evidence](../evidence/enhanced-editor/readers-exec-memory/lifecycle-emulator.json).

The exact fault component also passed once: RC0, observed 1 MiB Stack and all 24
lines, including all 53 cancellation phases and the complete existing negative
groups. Fast/Chip extent and zero-owned-byte assertions passed. Exact custody,
guest cleanup, original-file restoration, independent release and a separate
fresh idle check passed; recorded EMPTY state is fence 1695 with 87 archives.
The two component passes do not relabel either earlier incomplete combined run.
See [fault-component native evidence](../evidence/enhanced-editor/readers-exec-memory/faults-emulator.json).

The separate real-A1200 lifecycle packet is host prepared: 52 orchestration,
guest-model, service and factory checks, offline construction and validation
pass. It pins the exact independently released emulator candidate and observed
Stack. Its normal entry remains inhibited, with no live service, admission,
physical probe or test. Actual Safari access is currently blocked by the Mac
lock. Fresh browser, ownership, target, build and promotion checks remain required.
See [physical preparation, not acceptance](../evidence/enhanced-editor/readers-exec-memory/lifecycle-physical-prepared.json).
