# Optional whole-song persistent-reader producer

`paula_readers_song.c/.h` adds an opt-in software schedule producer over the genuine
opaque renderer/Paula setup, complete Paula audit, the same transferred sequence,
lookahead and sampler/readers boundary adapter. It owns a fresh queue and pool with
exactly two command records and eight persistent-reader records. It does not connect
editor PLAY, create a native activation backend, or qualify device timing or audio.
Existing synchronous, renderer, boundary and legacy song APIs keep their behavior.

## Inputs and borrowed lifetime

Before `pt_paula_readers_song_begin`, every nonempty project sample must already have
a genuine matching `sampler.current` version. Prepare those versions through the
existing sampler before beginning this owner. Missing or mismatched versions refuse;
the producer never promotes PCM or changes its document descriptor while the audit
or sequence borrows it. After actual initial READY, it retains one existing master
reference per step. Empty slots are skipped, including unselected empty slots.

Keep the project, sampler, full sample capacities, markers, tables, events, orders,
extensions and callback contexts alive and immutable through close. Every edit
between calls changes revision or sampler generation. A valid selection cursor may
move; an out-of-range cursor refuses. Source/control edits during callbacks and
callback reentry are forbidden. Calls are serialized. Before a begin owner exists,
begin allocation callback serialization is a caller obligation; subsequent detected
reentry latches failure.

Begin copies the configuration and ordinary allocator arguments. Their original
objects may be reused after that call, while their copied callback contexts remain
live. Outputs must be disjoint from known source/control capacities and allocations.
Explicit backend `context_bytes` is guarded. Opaque ordinary/Chip allocator contexts
remain caller-disjoint obligations. A returned arena recognized as overlapping a guarded extent is not released as
fresh ownership. Allocators must return fresh, disjoint storage; unenumerated opaque
extents remain caller responsibility.

The configuration combines render options, Paula caps, reader/cache budgets, an
immutable scheduled grid, backend declarations, a nonzero session, `absolute_start`
and `control_budget`. The session must never be reused across queue lifetimes.
Render and grid rates must match at 44100 or 48000 Hz; grid frequency is at least
that rate and generations match. `pattern_only` and `row_range` are unsupported.
The fresh map uses `previous=NULL`. Reader capacities must be exactly 2/8.
`control_budget` charges this owner and all genuine ordinary child allocations;
existing pool control and Chip representation budgets still apply. An allocator
name or callback does not establish physical Fast/Chip placement.

## Caller lifecycle

Initialize a real, disjoint owner slot to NULL, then call `begin(allocator, sampler,
project, config, revision, &owner)`. PENDING owns initial preparation. Advance with
`step(owner, revision, work, optional_status)`, using work 1..4096. Use `get` for a
guarded public status snapshot. No caller-supplied validation certificate or copied
opaque owner is accepted.

| Phase | Actual work and ownership |
| --- | --- |
| INITIAL | Genuine cancellable semantic preparation of the original project; initial READY is not Paula compatibility |
| RETAIN | Retain genuine already-current nonempty masters, then transfer actual startup ownership |
| AUDIT | Complete Paula audit on the actual sequence before output preparation |
| POOL_VALIDATE / OUTPUT_SETUP | Bounded genuine pool preparation, fresh queue/pool allocation, transfer the same audited and rewound sequence |
| NEXT / FORECAST | Original interval, lookahead in chunks up to 256 frames, actual live consumption and saved boundary |
| KEYS / LOWER / PREPARE / ENQUEUE | Exact prospective ACTIVE keys, genuine copied boundary lowering, bounded cache preparation and actual reference transfer |
| PUBLISH | Caller must explicitly attempt publication; step/get never submit |
| END | DONE; retained originals may still exist |
| DRAIN / ERROR | Stop new production; explicitly service independently owned domains |

A step performs one genuine phase operation. Audit work and forecast/live consumption
are bounded by the underlying 256 tick/frame operation; representation preparation
uses existing transition/action, 4096 master-byte or 256 representation-byte limits.
Finite metadata guards are additional work. These are work bounds, not a wall-clock
latency guarantee. Step never activates a backend, polls receipts or implicitly
publishes. The producer commits the prepared interval at enqueue and waits in PUBLISH
until the caller deals with that saved submission.

`command_mask`, `reader_mask` and `published_mask` identify record indices, not Paula
hardware slots. Published includes accepted or uncertain submission; it proves no
adoption, activation, command detachment or reader retirement. READY/cache ownership
also supplies no ACTIVE permission.

WAIT_ACTIVE retains the same prospective original and boundary. Obtain a positive
actual command/reader observation through an explicit service call before continuing.
A future replacement can close old-original admission while the backend still plays
the old full key; the producer never falls back to it. Cancelling an independent old
reader does not close its replacement. Refused cancellation of a reserved reader
before publication leaves prospective admission open.

WAIT_PRESSURE retains the original boundary. Service independently proven domains or
make allocator space available within the copied fixed budget, then advance again.
An insufficient configured budget requires cancel/drain/close and a separately
configured begin; a live owner's configuration and deadlines do not change. Command
detachment cannot retire its reader. Empty or known other-route-only boundaries need
no command slot and commit even while both command records remain held.

## Explicit publication and exact frames

`publish_next(owner, revision)` attempts only the oldest unpublished command. The
backend clock must be strictly before its original first counter tick. Actual
activation belongs to the declared backend's unchanged `[first,last)` frame window.
The original grid, start and every absolute boundary remain fixed through readiness,
WAIT_ACTIVE and pressure. No catch-up, retry with a changed epoch, one-frame shift or
deadline rebase is provided.

PENDING means the backend confirmed no acceptance and retained no new references.
Only an explicit later `publish_next` may retry in that same original window; step
and get do not. Accepted submission and uncertain acceptance retain their real
reference domains. Uncertainty, stale source or LATE latches failure and prevents a
new submission or automatic retry. Public `scheduled_result` records the underlying
classification even when the producer returns BACKEND for its sticky failure.

With `include_lead_in=0`, the genuine first fresh interval is zero-length: its initial
trigger is at `absolute_start`, not one tick later. Absolute terminal addition and
counter deadline conversion are checked before accepting their domains. Overflow
refuses without moving that original boundary.

## Independent domain service, cancellation and close

Call `service_command(owner, index, cancel, optional_receipt)` and
`service_reader(owner, index, cancel, optional_receipt)` for actual retained record
indices. The producer supplies each real ticket/action internally; the caller cannot
supply a proof or key. Each call performs at most one backend callback. Exact command
DETACHED can release the command record independently. Reader storage requires exact
RETIRED and zero referencing commands. A reader retirement proved first therefore
keeps its pin/cache until the last command detaches.

Uncertain or malformed observations grant no new ACTIVE permission and retain
ownership lacking independently exact release evidence. An exact detached/retired
domain envelope can release its own domain with an invalid terminal classification.
Independently exact proofs can still drain after sticky failure; a BACKEND service
result can coexist with that release. Do not infer release from the return code
alone. Sticky failure suppresses external receipt publication. For stale source use
NULL receipt output to drain without reading former source tables.

`cancel(owner)` stops new production, clears local forecast/preparation and cancels
unpublished queue domains once. Repeated cancel performs no backend polling and no
musical STOP. Submitted/uncertain domains still require explicit service.

`close(&owner)` requires a valid, live typed slot containing this actual owner (or
NULL). It cannot validate an invented pointer stored in arbitrary bytes. The slot
must be disjoint from the owner, source and other known extents. Close refuses while
any registered domain remains; it never polls, force-unpins or retries uncertain
work. Cancellation/close avoid traversing former source arrays, while their fixed
project/sampler controls and callback contexts must remain live.

An ordinary refused close retains the owner. Newly detected release-callback reentry
or fixed-control changes can instead consume it, set the unchanged caller slot to
NULL and return 0. Inspect the real slot: a NULL slot must not be used again. A
preexisting sticky failure permits otherwise quiet, independently proven cleanup.

## DONE, STOP and supported boundary shapes

DONE commits the empty renderer end and retains all still-owned readers. It sends no
STOP and proves no reader retirement. Natural renderer one-shot completion also does
not emit a STOP action. Explicit NOTE_OFF is an ordinary scheduled STOP.

`terminal_stop(owner, revision)` is a separate action after DONE. It uses exact
positive prospective originals at the unchanged terminal frame. STOP publication,
command detachment and reader retirement are distinct. A collision or expired
original deadline refuses without moving the STOP. A healthy exclusive producer's
normal end interval is positive; an initial zero-length F00 has no published
originals. A same-frame terminal collision is therefore not forced by the new fixture;
the generic ordered-queue guard remains required and no producer runtime collision
qualification is claimed.

Selected Paula triggers are the existing exact mono8/16/24 one-shot shapes with even
bounds and initial phase. Genuine lowering folds a TRIGGER plus its matching final
CONTROL into the final pitch/gain, preserving source/range/phase. Continuing CONTROL
and STOP resolve sample/channel from the actual registered original. Unknown kinds
and invalid tracks refuse even on ignored routes. Loops, stereo, interpolation,
SEGMENT, REPEAT and row-range restoration are unsupported for this Paula output.

## Qualification and native stack gate

Frozen production-v2/fixture-v5 passes three ASan/UBSan host groups once each: the
whole-song fixture, genuine boundary fixture and opaque Paula startup fixture. Six
recorded compile/run commands return 0, with 70 full dependency queries, 82 canonical
inputs,107 SDK inputs, three saved products and no generated inputs. Independent
saved-proof review is clear within the implemented fixture scope. The documentation
author read saved records and authored the fixture, but did not run tests or builds.

One pinned assertion-enabled 68000/software-float compiler-only build passes 40
recorded commands and 29 ordered full dependency queries, with 79 canonical inputs,
32 SDK inputs and seven exact runtime inputs. `PTPaulaReadersSongTest` is a 263416-byte
HUNK, SHA256 `7976a01b42b664505a89d2948adb97640912fd1a739a1c95235837a1fd6f4dfa`.
Both complete fixture markers survive, SDK assertions remain active, and independent
saved compiler-proof review passes. The product is never executed: native runtime,
emulator and physical testing are NOT_RUN. Root acquired no target reservation,
window, lock, TAKE/control or queued guest action for this slice. See
[saved software/compiler evidence](../evidence/enhanced-editor/paula-readers-song/README.md).

The authored fixture drives actual whole-song preflight/sequence/lookahead/lowering
for mono8/16/24 immutable current masters, four triggers, 16 tracks with unselected
tempo/delay, more than 20 continuing CONTROL batches at 2/8, delayed activation,
replacement, command/reader pressure, independent detach/retirement, DONE and explicit
STOP. It includes observed actual sequence transfer, cancellation phases, stale/freed
former orders, copied controls, selected cursor bounds, known aliases, genuine unused
PCM-capacity get/step guards, explicit PENDING acceptance, accepted-before-uncertain
references, two malformed observations, independent fault drain, original overflow,
ordinary/terminal LATE and a real child-release callback reentry. Final cleanup asserts
zero owned allocations and pins. These are the implemented assertions exercised by
the saved host proof, with the following limits.

Close through unused PCM padding is specifically UNCOVERED. The fixture does not
exhaust every callback/alias extent, malformed/uncertain state, budget refusal,
empty/no-sample/ignored-route project, 44.1-kHz producer route or native stack. Its
oracle uses shared genuine renderer/lowerer APIs and a serialized host backend model;
it checks integration/ownership without independently proving those algorithms.

Original frozen v1-v4 failures remain separate: v1 expected old-key admission after
a future replacement; v2 passed a poisoned owner value through a close handle; v3
expected the initial zero-length boundary to exceed start; v4 expected PENDING on the
successful terminal DONE transition. Later fixture corrections preserve their bytes
and results and do not relax production timing or ownership rules. Production v2
separately adds reporting of child/final release reentry through its close latch.

The constructor's local spans[6144] and samples[255] arrays have a source-only lower
bound of 67512 bytes assuming four-byte pointers and `size_t`, before other locals,
call frames and callbacks. This is not measured native `sizeof` or stack usage.
They structurally exceed a nominal 65536-byte 32-bit stack. The existing 65536-byte native
launcher stack is not sufficient qualification for this constructor. Native use needs
a separately qualified adequate stack or a metadata-storage redesign. Compiler/HUNK
proof alone does not establish stack adequacy, execution or timing. No native
activation backend, editor PLAY, Fast/Chip placement, DMA/IRQ/MMIO, emulator/device
run, audio or listening acceptance follows from this slice.
