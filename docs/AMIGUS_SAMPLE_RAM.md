# Bounded AmiGUS sample RAM adapter

`amigus_sample_ram` supplies concrete address allocation and upload callbacks for
`pt_sample_cache` and `pt_playback_pcm_upload_chunks`. Its register bus remains
injected. It does not bind a card, call amigus.library, install interrupts, start
voices or enable any native output.

The pinned public SDK at `d8c9a0429f41cd5f3dbadae34ef438e45c9c3718`
provides register facts in
[PlayWAVetable.c](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Tools/PlayWAVetable/PlayWAVetable.c).
The public descriptor exposes a wavetable base and reservation flag, but the
SFD has no sample allocation/upload operation. The implementation here was written
independently; upstream utility code is not incorporated. Input hashes and source
observations are retained in `evidence/enhanced-editor/amigus-sample-ram/`.

## Ownership and allocation

The caller supplies a verified, exclusively owned byte-address range. There is no
assumed RAM capacity or hard-coded division into stereo banks. Initialization
refuses an empty, unaligned or wrapping region without I/O. It also refuses any
region outside the25-bit address port documented by both pinned register maps;
that width limit does not prove the supplied region is installed or available. A fixed 32-descriptor
arena uses first-fit allocation, rounded to four bytes. Addresses do not move.
Fragmentation or descriptor exhaustion refuses allocation; the existing cache
may evict unpinned entries and retry. Rounding consumes physical capacity even
though the generic cache accounts logical sample bytes.

Keep the arena metadata in Fast RAM and at a stable address. The cache's existing
leases retain blocks while voices reference them; invalidation retires an old
version until its final lease ends. A completely uploaded block can report its
byte address and logical length. This does not substitute for a pinned voice
lease or establish a hardware voice's stopped state.

The entire cache lifetime requires an exclusive wavetable reservation. Losing
ownership requires voices to be stopped and all handles discarded before reuse;
reacquiring the reservation does not validate old card RAM. The reservation adapter now supports explicit PCM or wavetable selection; the
legacy open entry point remains PCM-only. Select WAVETABLE explicitly for this
cache. No combined resource masks are accepted.

## Upload semantics

The existing converter reads the immutable 8/16/24-bit master and selects one
source channel, emitting signed 8- or 16-bit bytes with explicit endianness.
Those bytes are the derived representation; the master retains its precision.
Each callback accepts at most 256 ordered bytes. Up to three carried bytes bridge
chunk boundaries. Full words write an explicit byte address at 0x14 followed by
a numeric MSB-first data word at 0x10. The adapter does not depend on an address
left over from a previous callback. The final partial word receives zero padding
inside the allocation; voice bounds must use the logical length.

A cache miss is published only after every chunk succeeds. Failed I/O poisons
that upload and the caller discards the unpublished resource. Failed address
writes never issue a data write. A fresh offset-zero LOAD resets upload progress
for safe reuse of an unpinned equal-sized allocation. The injected bus must be
synchronous, including failures: no deferred DMA or host-buffer references may
remain when a callback returns. A future asynchronous bus needs a different
lifetime contract and must not be plugged into this callback.

## Validation and remaining integration

Host ASan/UBSan checks cover exact conversions at several chunk sizes, all final
byte counts, guard bytes around physical allocations, fragmentation, descriptor
limits, rounding pressure, pinned old revisions, eviction, equal-size refill,
ownership refusal, out-of-order chunks and every register-write failure point.
Existing conversion, cache and AmiGUS transport checks also pass. The pinned
Amiga build records source dependencies, compiler/runtime hashes and binary hash.

Remaining: verify card/firmware capacity and physical Mini bus transactions,
qualify the separately implemented native binding on the actual card, and connect voices with confirmed
stop/transfer completion before releasing leases. The injected bus establishes
software behavior only. There is no real card access, device upload, audible
playback, physical performance or physical acceptance from these fixtures.

The native fixture also passes shared030 in its coordinated 90-second window
(run `render-files-1790473124808998000`): RC0, one Fast metadata allocation, zero
final owned bytes and budget refusal without Chip fallback. All four DMA channels
were off before exact owned cleanup and explicit resource release. No timeout,
retry or reset was needed. This still uses the injected bus, never actual RAM
uploads to an AmiGUS.

## Reservation-bound cache owner

`amigus_wavetable_cache` borrows an explicitly open WAVETABLE reservation and
holds its access lease for the entire cache lifetime. Attach refuses a PCM-only
reservation, an existing access lease, an invalid region or missing ownership.
It performs no register writes. Failed attach leaves reservation access unchanged.
The core/native reservation callbacks now receive an explicit single resource;
the same selected flag is used for support checks, ReserveCard and FreeCard.
Resource-specific busy codes are recognized. The old open entry point and the
discovery PCM count retain their existing behavior. Combined masks are refused
because the public driver can acquire individual parts before another part fails.

Acquire and address lookup require current ownership, including cache hits that
need no upload. Detected ownership loss is latched until detach; making the
predicate true again cannot resurrect cached card contents. A failed upload
discards only its unpublished resource and retains the card/library owner.
Invalidate retires pinned old versions while a later version gets a separate
allocation. A lease may be released only after its voice/transfer is confirmed
stopped; no hardware voice-stop mechanism is implemented here.

Detach retires all entries and blocks new acquisitions/address lookups. It refuses
while any sample lease remains pinned, retaining the reservation and library.
After the final unpin, retrying detach releases access and leaves the borrowed
reservation open for its owner to close. Do not end that access lease externally.
The cache and its callbacks must remain alive until detach succeeds. No hardware
stop, drain, interrupt teardown or audible silence is inferred from these calls.

Validation uses fake-library calls plus injected memory and preserves the entire
24-bit source while deriving16-bit cache bytes. Tests cover wrong-resource and
attach failures, cache-hit ownership loss, pinned generations under pressure,
partial upload failure, repeated detach, and exact reserve/release resource
identity. Production native descriptor inspection is tested against synthetic
PCM-only, wavetable-only and unsupported card descriptions; it does not access
real descriptors or invoke the production library callbacks.

The ownership integration passes all10 AmiGUS host sanitizer tests (5.798s) and
native shared030 run `render-files-1790473732795430000` within a separately
coordinated90-second window. RC0, one Fast metadata allocation, zero final owned
bytes, no Chip fallback; completion, all four DMA off, exact cleanup and explicit
release verified. PCM session and discovery probes still cross-build. This run
uses fake library/bus callbacks; only descriptor inspection invokes a production
native callback on synthetic data. It does not test ReserveCard/FreeCard against
a real library or establish native MMIO/voice-stop behavior.

## Sampler revision bridge

`src/editor/sampler_wavetable.c` binds one initialized sampler/project to a
dedicated empty attached wavetable cache. It pins the immutable master for each
synchronous upload, then releases the source pin on success or failure. The
derived device lease has an independent lifetime: a playing voice retains its
old bytes until confirmed stopped and explicitly unpinned. Initial document
samples are promoted by the existing bounded sampler allocator without changing
their precision, saved bytes or undo history.

Before each acquire or address lookup, the bridge checks sampler generation,
sample-table identity and slot count. Any change retires all cached copies and
advances a separate64-bit cache revision. This is deliberately conservative:
metadata-only edits, undo/redo and table growth also rebuild representations.
Failed edits do not advance the sampler generation and can retain cache hits.
Retired pinned blocks remain allocated; their lease cannot authorize a new
trigger through the bridge's address lookup. A voice already playing retains
its previously supplied address. Revision exhaustion refuses reuse.

All edits must use sampler APIs, and callbacks must not reenter the editor.
In-place mutation outside the immutable-master contract is not detected by this
bridge. Close before document replacement or sampler reinitialization; a new
provider refuses an already populated cache. A busy close blocks new work but
keeps both contexts and reservation access alive until the last playback lease
is released. Then close detaches the backend; the reservation owner closes its
library separately. A new document may bind only after that cleanup/re-attach.

Host tests cover budget/allocation refusal before promotion, exact cache hits,
failed edits, gain and loop edits, undo/redo, slot growth/table undo, source-slot
replacement, failed uploads, old pinned byte preservation, revision exhaustion,
close refusal and another document's fresh binding. Exact enhanced-project saves
retain low24-bit source data throughout promotion and after undo; all allocator
ownership returns to zero after cleanup. The five sampler sanitizer fixtures
pass (20.824 seconds). This connects the software sampler API to the cache owner;
the native editor still does not instantiate an AmiGUS bus or voice dispatcher.

The sampler bridge also passes shared030 within its separately coordinated
90-second window, run `render-files-1790474301354327000`: RC0,28 Fast allocations,
zero final owned bytes, and budget refusal without Chip fallback. Completion,
all four DMA off, exact cleanup and explicit release were verified. No timeout,
retry or reset was needed. Native evidence still uses a fake library/bus and
does not establish card upload, voice stop, audible playback or physical timing.

## Bounded voice lease ownership

`src/editor/wavetable_voices.c` owns up to16 software voice slots around the
sampler bridge. The injected start/stop interface is a driver contract, not
native voice programming. Bind to an exclusive bridge with no outstanding
leases, and exclusively owned, initially stopped voice IDs. The owner and all
borrowed contexts must remain alive until close succeeds. Calls run on one
serial control thread; callbacks must not reenter or edit the sampler.

Retrigger first acquires a current representation. If allocation/upload fails,
the old voice remains playing with its lease. Once a candidate exists, the
owner attempts the old voice's stop once. Pending or failed stop keeps the old
lease and releases only the unstarted candidate. Nothing is queued: a later
explicit trigger retries against the current master. After confirmed stop, the
bridge validates the candidate's revision/address again before start.

The owner records its pin before invoking start. Only return1 confirms start;
any other start result is uncertain and retains the pin until an explicit,
confirmed stop. A stop callback may return1 only after that voice can no longer
read sample RAM. Sending a stop command alone is insufficient. Pending/failed
stops never free a voice's sample. Lost cache ownership blocks new triggers but
does not discard existing voice pins; safe stop recovery remains the injected
driver's responsibility.

Close blocks new triggers and attempts each held voice once per call. It retains
the bridge and reservation access while any stop remains pending/failed. After
all stops are confirmed, it detaches the bridge; the reservation's outer owner
then closes the library. No polling loop, automatic restart, forced cleanup or
assumed physical stop acknowledgement is hidden in this API.

The host fixture tests16 simultaneous leases sharing one copy, same-sample
retrigger, edited/retired copies under allocation pressure, pending/failed stops,
retry after undo, uncertain starts, lost ownership and bounded repeated close.
Enhanced saves preserve exact24-bit master bytes. This is software lifetime
coverage; pitch/loop/volume commands, native voice register dispatch, actual
stop acknowledgement and real card capacity remain integration requirements.

This voice owner passes host ASan/UBSan and shared030 run
`render-files-1790475253667152000` (RC0 within90s). The native fixture uses40 Fast
allocations with zero final owned bytes and budget refusal without Chip fallback.
Completion, all4DMAoff, exact owned cleanup and explicit release were verified.
See `evidence/enhanced-editor/wavetable-voices/`. The native editor still does not
instantiate this owner or an AmiGUS output adapter.

## Voice command preparation and register-width audit

`amigus_voice_plan` prepares a value-only command for the lease owner's injected
start callback. It performs no I/O, source reads or allocation. The project and
cache retain responsibility for validated master contents. The caller supplies
resolved pitch as a rational frame rate, resolved volume0..64, linear pan0..256
and an offset measured in source frames. Finetune, velocity and tracker effects
must be resolved by the sequencer before this call; the planner does not apply
those a second time.

The planner supports one selected source channel in an8/16-bit playback cache,
with no implicit stereo downmix, Paula padding or source precision change. It
checks exact logical cache length, all byte/frame arithmetic and register-width
bounds. Forward loops and one-shot playback are supported in the software plan;
ping-pong/crossfade are refused. All start/loop/end pointers must be even, including
8-bit playback: odd offsets/loop bounds or odd one-shot lengths are refused,
never rounded or silently padded. Changing the explicit playback representation
to16-bit can represent odd frame positions without altering the master.

Playback rate is floor(numerator *2^30 /(192000 *denominator)), using64-bit integer
arithmetic; zero, quantized-zero and rates above192000 are refused. Left/right
levels use rounded linear balance with65535 as full scale. This balance is a
software mixing policy; hardware loudness has not been measured. Control flags
encode resolution, forward loop, interpolation,16-bit byte order and start only.
Invalid input leaves the output unchanged. Trigger preflight rejects invalid
metadata before upload; the acquired address-specific plan is checked before
stopping existing playback. The callback receives the prepared plan while its
sample lease is held.

Register facts come from the pinned SDK's `AmiGUS_Register_Map.xlsx`, sheet
`Hagen Register`, B88:F117, and mini map `SHIVA Register`, B89:F118. Both specify
25-bit sample addresses and even voice pointers. The utility supplies the rate
formula. Source hashes and exact cell references are retained in
`evidence/enhanced-editor/amigus-voice-plan/source-observations.json`.

The maps do not explain inclusive/exclusive end-pointer behavior or establish
that control-bit readback fences all sample-memory reads. The plan therefore
keeps an explicit HALF-OPEN software end bound; mapping it to an actual end
register requires further protocol verification. The utility and maps also
conflict on envelope-enable bit4 versus bit5; this code enables neither. Native
start/stop dispatch stays disabled. No physical capacity or playback acceptance
is claimed from this preparation layer.

The preparation layer passes11 related AmiGUS host fixtures (5.692s), the final
three wavetable integration fixtures (8.238s), and shared030 run
`render-files-1790475765845129000` (RC0 within90s). The native fixture returns all
40 Fast allocations; no Chip fallback. Completion/all4DMAoff/exact cleanup and
explicit release verified. Evidence is in `evidence/enhanced-editor/amigus-voice-plan/`.

## Resolved sequencer dispatch

`wavetable_dispatch` now consumes successful audited `pt_render_plan` batches
through the injected voice owner. It handles ordinary mono TRIGGER, STOP and
CONTROL operations. Every action preflights before the first device callback;
unknown/private PCM descriptors, stale bridge revisions, invalid channels,
unsupported ranges and segment/repeat operations refuse the whole batch. PCM
identity is compared with the current project's descriptors before dereference.
The caller must synchronize the bridge and capture its revision when building
the plan, preserve the immutable project/sequence, and consume the preceding
interval before dispatch. This layer does not provide a scheduler.

The renderer's Q32 step maps directly to floor(step *output_rate /768000),
preserving fractional pitch until the final hardware-rate quantization. Only
44100/48000 renderer clocks and resolved source rates up to192000 are accepted.
Already-resolved Q16 left/right gains map independently to16-bit levels. Panning,
finetune, mute, velocity and tracker volume are not applied again. CONTROL uses
an optional synchronous callback that changes only rate/levels: it must preserve
phase and existing sample ownership. A trigger starts with its captured gains;
the renderer's subsequent CONTROL establishes final gains before the next span.

Successful triggering takes a current cache lease and validates the actual
address before stopping/replacing an old voice. Any invoked start owns its lease
until stop confirmation, including an uncertain result. During runtime failure,
dispatch blocks new work and makes one cleanup stop attempt for each held voice.
A stop attempted by the failing operation can therefore receive one additional
cleanup attempt; no polling loop occurs. Unconfirmed stops keep pins and the
reservation. The caller must discard the failed sequence and close/rebind; a
partially applied plan must never be retried. Preflight refusal returns0 without
device callbacks and leaves existing voices playing; the caller chooses an
explicit Stop/close before abandoning that sequence.

Stereo expansion, segment playback, boundary-delayed cross-source repeats and
private EFx banks remain unsupported by this dispatcher. They are refused rather
than silently mapped to simpler playback. Existing Studio paths keep their
fuller software behavior. Output plans retain half-open byte bounds; real-device
endpoint interpretation, stop fences and native bus wiring remain unverified.
Native editor PLAY/output remains disabled for this adapter.

Validation generates actual16-channel renderer plans and checks note starts,
phase-preserving pitch/gain changes, Stop/retrigger and both control/start
failure retention. Host sanitizers pass; shared030 run
`render-files-1790476398105279000` returns RC0 within90s with50 Fast allocations
and zero final owned bytes. All4DMAoff/exact cleanup and explicit release were
verified. See `evidence/enhanced-editor/wavetable-dispatch/` for scope and hashes.

## Whole-sequence capability check

`pt_wavetable_preflight` silently traverses the complete immutable audited
sequence before a caller begins wavetable output. The same private capability
checker validates both this pass and live dispatch batches, including descriptor
identity, mono trigger geometry, resolved control values and supported operations.
Late stereo notes, segment operations and cross-source repeat changes are found
before any device callback. The report distinguishes format, geometry, control,
operation, renderer and allocation failures; it records the interval count,
consumed frame count and first offending action/channel/kind when available.
Renderer failures retain the original `pt_render_result`.

Measurement and traversal use the existing mandatory tick/frame budgets. The
plan and sequence are allocated through the caller's allocator and freed on every
exit. Silent pre-roll and retained/delayed rows are included; phase advances in
blocks of at most256 frames. Renderer validation can inspect source PCM, but the
pass neither mixes audio nor writes masters, uploads samples, reserves a device
or invokes voice callbacks. The eventual driver's control capability is explicit.

A successful report applies only to those immutable project/options/format inputs.
It is not a playback session or a cache-capacity promise. Edits require another
pass; current revision and real-address checks remain necessary during dispatch.
A higher-level owner still needs to enforce analysis before output, hold source
lifetimes and sequence timing, and handle silent pre-roll/range-start policy.
This milestone does not wire native PLAY or establish real AmiGUS semantics.

Validation: final host ASan/UBSan fixture PASS5.851s; native shared030
`render-files-1790477101421455000` RC0 within90s,73 Fast allocations and zero final
owned bytes/no Chip fallback. All4DMAoff, exact cleanup and explicit release
verified. See `evidence/enhanced-editor/wavetable-preflight/`. No native output or
physical acceptance is claimed.

## Owned wavetable sequence lifecycle

`wavetable_song` wraps the mono dispatcher with an enforced full-sequence
capability check before source promotion or device callbacks. On success it
borrows an already-bound idle voice owner exclusively and records a session
marker so a second song cannot take that owner. The preflight report identifies
actual triggered sample slots; only those masters are pinned for the session.
Unused project samples are not promoted or cached. Workspace allocation and
source promotion use caller/sampler allocators and existing memory ceilings.
Failed open preserves the output handle and voice ownership, releases temporary
pins/workspaces, and never starts a voice. A promoted sampler.current copy can
remain after a later failure; it is still the intact authoritative master.

The public protocol is next -> consume elapsed frames in blocks of at most256
-> complete. The caller supplies the clock; consume does not wait or produce
PCM. Complete applies the audited plan only at the interval boundary. Initial
omitted lead-in can contain multiple zero-frame spans. Options and conversion
format are copied. Row-range playback explicitly refuses until a phase-correct
silent pre-roll/start policy exists for hardware voices.

Project pattern/order/sample arrays are borrowed and must remain immutable:
callers must close before editing or replacing them. Each step detects sampler
revision, table identity and project-header/settings changes before using the
sequence. This is not a deep project snapshot and cannot detect arbitrary
in-place array writes. Native editor edit guards remain future integration.

Natural end, cancellation and stale/runtime failures request bounded confirmed
stops. Failed dispatch may attempt its own stop cleanup before the session's stop
pass. No polling loop occurs. The sequence is discarded immediately; all master
pins/controller state and any unconfirmed device leases remain until voice close
and cache detach succeed. Close returns pending without freeing the controller;
only a successful close nulls the caller's handle. The outer reservation remains
caller-owned. A poisoned session cannot retry a partial plan or restart voices.
These are injected-driver ownership semantics, not verified real stop fences,
native PLAY wiring, device scheduling or hardware output.

Host ASan/UBSan PASS5.940s and shared030 run
`render-files-1790477656284461000` RC0 within90s validate these software lifetimes.
The native fixture returned all115 Fast allocations, with no Chip fallback.
All4DMAoff/exact cleanup and explicit release verified. Evidence and limitations
are recorded in `evidence/enhanced-editor/wavetable-song/`.

## Editor mutation barrier

`editor_wavetable` attaches the song owner to a veto-capable editor change
barrier. An unconfirmed stop returns0 and retains the song, pinned masters,
device leases, editor/document and guard context. Stop/detach are retryable and
never poll or force-free. Attachment refuses the existing Studio/legacy hook;
that synchronous hook remains supported without changing its callback signature.

`pt_editor_prepare_change` and `pt_editor_dispose` now return confirmation.
Every guarded controller mutation checks it before calling pattern, sample or
song operations; a refusal returns edit conflict without changing the project
or journal. Platform sample imports honor it before importing. Native New/Load,
quit, bounce and Stop paths also honor it. External owners must check these
returns before storage replacement/free and detach before editor reinitialization.
This adapter does not instantiate native AmiGUS output or alter the classic layout.

The wavetable session ignores selected-channel cursor movement when checking
project metadata, so ordinary navigation continues without stopping playback.
Actual sample/project edits still close the entire song before mutation. Once
stop is confirmed, a repeated edit/undo/redo proceeds against the original master
and its chronological journal. A future playback restart needs a fresh bound
voice/cache owner; the outer card reservation remains the application's concern.

Validation uses a Git-index source export so unrelated local display changes
are excluded. Host sanitizer fixtures cover editor wavetable veto/retry ownership,
legacy editor guard and existing Studio queued/pinned lifetimes. The cross-build
also syntax-checks the staged native main with the pinned generated bitmap font;
that is not a full native editor runtime or physical-output acceptance test.

Host ASan/UBSan: all three fixtures passed in25.946s. Shared030 run
`render-files-1790478371042795000` returned0 within90s, with146 Fast allocations
and zero final owned bytes/no Chip fallback. All4DMAoff, exact cleanup and
explicit AmiConnect release verified. Evidence: `evidence/enhanced-editor/editor-wavetable/`.

## Row-range reconstruction foundation

The audited render sequence now publishes an interval-start software snapshot
before consumption. Resuming its copied voices reproduces the same exact24-bit
PCM after silent pre-roll, including fractional phase and loop-relative position.
Snapshots are bounded borrowed state, not independent sample owners or hardware
command plans. The wavetable session still refuses row_range: a future restore
plan/driver must preserve the full position or refuse before any output. The
current initialized-trigger translator must not silently discard snapshot phase.
See `evidence/enhanced-editor/render-snapshot/` for host/native evidence and limits.

## Exact software restore plans and capability gate

`pt_amigus_render_restore` preserves an active mono one-shot/forward voice's
sample position as an absolute cache-byte Q32 cursor alongside its original
aligned bounds. It validates phase/cycle and bounds without reading master PCM.
The cursor may be odd/fractional; it must never be discarded or rounded to feed
an ordinary start. `pt_wavetable_restore_preflight` requires explicit exact-restore
capability, resolves every active borrowed descriptor and validates the entire
snapshot before a future caller may acquire/upload/start. Source/cache leases
remain mandatory; these pure functions do not own storage or publish output.

The injected voice API and session integration described below consume these
restore plans. This is software preparation, not native register lowering or
verified endpoint/phase hardware support. Host sanitizer checks pass; native
restore fixture builds and dispatcher syntax checks pass. No new emulator run
was attempted during the retained renderer recovery hold. Evidence is in
`evidence/enhanced-editor/amigus-restore/`.

## Injected restore ownership

The optional voice API `restore` callback consumes a complete exact-cursor plan;
ordinary `start` remains separate. `pt_wavetable_restore_dispatch` accepts only
an idle bound owner at the current bridge version. Full snapshot capability/source
checks precede cache acquisition, and every active cache lease and actual-address
plan is ready before the first restore callback. Cache sync may retire stale
entries. Failed preparation invokes no voice callback and releases all temporary
leases; unpinned loaded caches or promoted unchanged masters may remain.

Each callback receives a lease already owned by its voice. Any result other than1
is uncertain: the owner closes, unstarted candidates are unpinned and started
voices receive one stop attempt. Pending/failed stops keep their leases and card
reservation alive until an explicit confirmed retry. The callback must copy the
whole plan synchronously and never mutate/reenter sampler state. Source identity,
cache location and ownership are rechecked; all pinned caches resist eviction.
This injected driver integration does not enable native MMIO or real card playback.
Row-range session integration is described below.

Restore-owner host sanitizer and staged native build checks passed. Shared030
run `render-files-1790480037972486000` returned0 within90s with156 Fast allocations,
zero final owned bytes, all4DMAoff, exact cleanup and explicit AmiConnect release.
See `evidence/enhanced-editor/wavetable-restore-owner/`. These injected callbacks
do not qualify a real restore register implementation.

## Row-range session integration

`pt_wavetable_session_preflight` validates both the full sequence and its first
emitting snapshot before source pins or output. Range sessions require an explicit
exact-restore callback. Silent pre-roll advances software state only; it never
uploads or invokes voice callbacks. Active snapshot sources and later emitted
triggers are pinned, while samples that already ended are omitted. `next` acquires
and restores all active voices at the first emitting interval before any of its
frames elapse. The original renderer controls interval timing and the normal
complete/dispatch protocol resumes afterwards, with confirmed-stop ownership on
natural end, cancellation and failure. See `wavetable_song.h` for caller duties.

Evidence in `evidence/enhanced-editor/wavetable-range/` covers fractional cursors,
silent pre-roll, future triggers, final spans and pending/uncertain stop ownership.
No native register, clock, audible output or physical AmiGUS acceptance is implied.

## Incremental preflight traversal

The synchronous preflight wrappers now use the resumable begin/step/close engine.
PENDING is distinct from COMPATIBLE; partial masks never qualify playback. Begin
performs static project/PCM validation and allocates the workspace plus renderer.
Timeline measurement now yields after at most256 ticks, and command traversal
separates next/snapshot, at most256 silent frames, and complete/capability checks.
Cancellation frees both allocations without any source pin, cache or device side
effects. Terminal success/refusal repeats without further advancement. The inputs
remain immutable borrowed storage, with options/format copied at begin.

The initial scans and command-time PCM checks are still synchronous, and native
song/editor start still uses the synchronous wrappers. This milestone provides a
cancellable analysis primitive; it does not claim responsive native PLAY or a
hardware scheduler. Evidence: `evidence/enhanced-editor/wavetable-preflight-step/`.

## Preparing song/editor ownership

Song begin publishes a PREPARING owner with copied options and immutable borrowed
project storage. Steps guard generation/header/bridge revision, advance one
analysis operation, then (only after complete compatibility) pin at most one
selected master per call. A one-time preflight transfer resets the completed
non-mutating sequence for playback without remeasurement. Next/consume/complete
stay blocked until readiness. The synchronous open API wraps these steps.

Cancellation/refusal before readiness frees preparation/source leases and releases
the song claim while keeping the idle backend bound for caller reuse/close. After
readiness the existing confirmed-stop and cache-detach lifecycle applies. The
editor barrier cancels a pending owner before edit, Stop or disposal. Partially
promoted unchanged masters can remain sampler-owned; no source downgrade occurs.
Static/sync PCM scans and sample copies remain synchronous. Native PLAY, real-time
scheduling and physical card semantics remain unverified. See
`evidence/enhanced-editor/wavetable-preparing-owner/`.

## Exact callback success and failure publication

The sample-RAM arena and reservation-backed cache require exactly1 from ownership
and synchronous write callbacks. Negative errors and unexpected positive values
fail closed; an upload with either address or data write failure cannot publish a
cache lease. Ownership faults remain latched, and active leases keep the access
reservation until unpinned. Host regressions and native fake-bus fixtures pass;
see `evidence/enhanced-editor/amigus-callback-status/`. This does not qualify real
card access, interrupts, output or physical memory capacity.


## Native sample-RAM bus candidate (2 October 2026)

`src/native/amigus_ram_bus.c` now provides the production volatile32-bit bus
callbacks for the separately identified Mini hardware0/firmware7ea663e7. Bind
performs no I/O and requires an exclusive WAVETABLE reservation without an existing
access/interrupt owner. The stable bus context, library/card descriptor and
reservation outlive the cache. Bind before cache attach; the ownership predicate
permits the reservation-only attach check, while every store requires exactly one
access lease. Clear refuses until cache detach releases that lease.

The caller supplies an independently verified four-byte-aligned region inside
the25-bit address space. This bus selects no installed RAM capacity and does not
probe addresses. Address0x14 and data0x10 stores must alternate; each whole word
must fit the supplied region. Invalid ports/pairing/ranges, lost reservation,
changed descriptor/base or an interrupt owner latch refusal before further writes.
Restoring a lost predicate cannot resurrect the old bus. Numeric stores are
explicitly volatile and synchronous at the CPU level. Their return value is not
independent proof that device RAM accepted/completed the transfer; qualify those
semantics before connecting a live editor cache. No voices or interrupts start.

The pinned SDK at `d8c9a0429f41cd5f3dbadae34ef438e45c9c3718` documents a
32 MB Mini design capacity (`README.md` lines65/130 and
`Documentation/AmiGUS_mini/AmiGUS mini.guide` lines102/183–184). This documented
capacity is distinct from an independently qualified usable region on the
installed card. The library descriptor has no capacity field. The inspected SFD
has discovery/reservation/interrupt operations, but no upload completion or
ordering fence. `PlayWAVetable.c` demonstrates sample writes and32 voice-control
writes without a completion/readback acknowledgment; releasing reservation
metadata does not confirm that every hardware voice stopped. The successful
read-only global-status probe below cannot establish these remaining contracts.

Three host ASan/UBSan groups pass: native descriptor/ordinary-register storage,
existing bounded sample-RAM allocation/upload, and wavetable cache ownership.
The new fixture covers17 bind refusals, invalid/data-before-address stores,
reservation loss between address/data, sticky refusal,25-bit upper boundary,
retained access while a cache lease is pinned, and intact24-bit master conversion
to a padded16-bit representation through the actual cache callback chain. The
host uses a scalar SDK ABI shim; it does not establish68k layout. The pinned
compiler builds `PTExecAmiGusRamBusTest` using the real SDK via
`tools/build_amigus_sample_ram.py --ram-bus --cc <pinned compiler>`. Native
execution is pending fresh coordination after Scott's independently released
window. Hardware capacity/upload/readback/ordering/voice-stop/output/listening
remain open.


The exact RAM-bus fixture now passes shared030 run1790927892937497000 RC0 with
the real SDK and one Fast allocation/zero owned bytes/no Chip fallback. Separate
independent1790927959178348000 passed originalsolePID79263/profile/localhost/
running68030/all4DMAoff/exact run+launcher absence without recovery, then explicit
RELEASE. See `../evidence/enhanced-editor/amigus-ram-bus-native/`. The production
volatile callbacks operated on ordinary allocated memory, never device RAM.

## Read-only wavetable qualification candidate

AmiGUSTest0.9 adds `--wavetable-status`. It reserves only WAVETABLE under the
verified amigus.library1.1 contract and reads exactly eight global IRQ/mask words
at offsets0x00 through0x0e. It never selects a voice bank, writes a register,
uploads samples, resets hardware, installs interrupts or unloads AHI. Each read
checks the stable lease/card/base and observed Mini hardware0/firmware7ea663e7;
loss latches refusal. Own release is independently confirmed using the library's
NULL-owner query before closing; uncertainty retains Task/library/owner.

The pinned Mini workbook access facts and hash are recorded in
`evidence/enhanced-editor/amigus-wavetable-status/source-review.json`. Data,
address, reset and bank-selector registers are write-only in this map. There is
no documented sample-RAM readback or usage counter here. Snapshot values alone
cannot establish installed capacity, upload completion, ordering or stopped
voices. Host sanitizer/native build checks use ordinary synthetic storage;
exact shared030 run `render-files-1790931158760994000` passes the reader fixture
RC0 and real missing-library mode RC5. Its initial independent cleanup failure is
preserved alongside separately guarded empty-directory recovery and final
independent release checks. See `../evidence/enhanced-editor/amigus-wavetable-status/`.
Separate realMini run1790982188208138000 now passes eight global reads (all0000)
and positively confirmed own release, exact RAM cleanup, separate physical
absence, unchanged idle emulator return and independent release check. Safari
remained View Only with no control taken. See
`../evidence/enhanced-editor/amigus-wavetable-status-physical/`. This does not clear
another task's playback/listening gate or qualify sample-RAM capacity, transfer
completion/order, all-voice stop or audio. No sample upload occurred.
