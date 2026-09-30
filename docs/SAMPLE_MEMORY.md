# Sample memory and playback ownership

The enhanced project owns the authoritative sample, including its declared
8/16/24-bit precision, loops and slices. PCM is represented by signed `int32_t`
elements in the declared precision; 24-bit samples retain their low eight bits.
Playback representations must never become the project's source of truth.

## Current implementation status — 30 September 2026

The sections below retain milestone history. Earlier statements such as “not yet
wired” may be superseded by later sections; this table separates the present
software implementation from the remaining integration and hardware gates.

| Requirement | Implemented software | Still required |
| --- | --- | --- |
| Authoritative8/16/24-bit masters | Queried, bounded native Fast-RAM pool; precision-preserving sampler versions, processing, history and source pins. Fast-equipped machines fail rather than spill enhanced allocations into Chip RAM. | Physical memory-pressure/endurance measurements. |
| Optional Paula copies | Selected8-bit Chip allocations, pinned during use; restart reload/reuse, eviction and guarded stop/release. Enhanced sample audition and one-to-four-track song playback preserve master precision; song preparation excludes unreferenced masters from both Fast staging and Chip caches. Dedicated arbitrary-track cache and injected four-voice owner preserve stable slots and retain leases through uncertain start/control/stop. Full16-track shared-sequence capability gate validates explicit clock/stereo/volume and supported sample geometry before output. Bounded caller-workspace dispatch prepares all trigger leases before any injected callback; refusal preserves existing readers and runtime failure blocks replay while retaining unconfirmed readers. A cancellable session retains the same fully checked sequence, selectively pins masters in bounded copy steps, claims exclusive voice ownership and retains pins through failed stop/quiescence. Its editor barrier vetoes edits, imports, undo and disposal until confirmed cleanup. Separate preparation/apply retains candidate leases and supports optional staged song/editor output without cache conversion during apply. A private prepared-master Chip job now converts at most256 output bytes per step and publishes only on completion; staged-song wiring remains pending. | Shared mixed-backend scheduling and native dispatch beyond the one-to-four-track Paula bridge; physical sound/performance acceptance. |
| AmiGUS wavetable copies | Versioned evictable resource pool, sampler revision/master-pin bridge, bounded 16-voice lease owner with validated rate/loop/address plans, mono sequencer trigger/control/stop dispatch, preflight-gated master-pinned sequence ownership, editor veto/retry barriers and injected callbacks, 8/16-bit conversion and bounded address allocator/chunk uploader behind an injected register bus; failure/pinning tests. | Verified card capacity, native bus binding and voice dispatcher; explicit wavetable reservation/cache lifetime now implemented and tested with fake callbacks. Injected register tests do not establish real card access. |
| Direct24-bit Studio | Master-pinned mixer, audited sequence, sampler/editor ownership, bounded queue/pump and integrated PCM-session owner with capacity-bounded prefill and explicit start acknowledgement; optional reservation lease retained through reset and adapter quiescence. Up to16 voice slots are a software bound. | Native PLAY/output wiring, verified hardware capabilities and sustainable physical voice-count/timing evidence. |
| Master-preserving persistence | Bounded enhanced-project/sample saving and explicit classic conversion; private EFx exports/bounce never replace masters. Explicit new-file recovery snapshots and staged identity-checked restoration preserve all master precision. | Native opt-in configuration, idle scheduling, bounded retention, read-only discovery, explicit recovery UI and exact save are implemented and emulator-tested; preferences-panel controls, real crash/endurance and physical acceptance remain. |
| Recording masters | Bounded full-precision collector, exact negotiated-format gate, injected input lifecycle, exclusive PCM/interrupt ownership, editor mutation/disposal veto and retryable undoable publication. Earlier recording owners passed native Exec fixtures; the new exact-format gate passes host tests and Amiga builds, with emulator execution pending after a prelaunch identity refusal. | Actual input capability negotiation, native input register/backend and controls, duplex ownership if needed, physical capture quality/overrun evidence. |
| Playback invalidation | Revision/settings keys, retired active leases and stop-before-edit/undo/dispose guards. Private EFx banks are discarded on stop and rebuilt on restart. | End-to-end real AmiGUS voice/transfer ownership, once the device adapter exists. |

AmiGUS PCM reservation/FIFO/session code has fake-port/library and native software
fixtures, plus an absent-library discovery check. Those results do not establish
positive card access, real MMIO, streaming output or physical acceptance. Native
Studio/device output remains disabled. See `AMIGUS_STUDIO_TRANSPORT.md` for the
precise boundary. Private EFx supports its explicit classic mono8 subset;
ordinary Studio retains direct16/24-bit masters and never silently reduces them.

## Implemented native master allocation

`src/native/master_memory.h` supplies the native editor's document, donor
sample documents, sampler versions and undo, song storage, editor/history
structure, import buffers and bounded save/export workspaces. They share one allocation
ceiling. Initialization queries Exec's installed Fast RAM and currently free
memory; it never assumes the ACA1234's nominal capacity is available.

When Fast RAM exists, all these allocations request Fast RAM explicitly.
Exhausted or fragmented Fast RAM fails the operation instead of spilling into
Chip RAM. A Chip-only machine may use Chip RAM above the reserve. The pool
reserves 256 KiB of currently free Fast RAM or 512 KiB of Chip RAM, respectively.
Every allocation rechecks current free memory as well as the shared ceiling;
Exec allocation failure is also handled. Reserves are headroom, not a guarantee
against concurrent allocations by other applications.

Size prefixes track exact allocation sizes, including overhead, for `FreeMem`.
Sampler history has an additional ceiling of half the available pool at editor
initialization; song history has one quarter. Both still compete within the
same shared pool. Failed imports, edits and conversions use the existing
transactional failure paths and preserve the old master. Save/export failures
preserve the current project. Generic portable-core defaults remain usable by
host tools; the native editor explicitly supplies this allocator.

## Playback-cache contract and remaining work

The following is the required architecture, not a claim that every backend is
already implemented:

- Paula: generate an optional signed 8-bit Chip-RAM representation only for
  samples needed by playback. Pin it while DMA/replay references it. Stop or
  transfer ownership safely before releasing it. Keep pattern/metadata storage
  out of Chip RAM where the replay integration permits.
- AmiGUS wavetable: upload optional 8/16-bit representations to card RAM; treat
  card addresses as evictable playback resources, never as master storage.
- Studio: read the Fast-RAM 24-bit master directly for the CPU mixer. A wavetable
  conversion must not reduce the stored master. Streaming and physical
  performance remain separate acceptance gates.
- Identify caches by sample identity/version and conversion settings. Master
  edits, undo/redo, slot replacement and loop changes invalidate affected
  representations. No subsequent trigger may use a stale version. Active
  voices must finish against pinned data or stop safely before replacement.
- Under pressure, evict unpinned derived representations and rebuild on demand.
  Do not discard masters to make room for caches. Report a shortage if no safe
  eviction/allocation is possible.

The classic Paula bridge now omits payloads of instruments unreferenced by any
stored pattern from its private Chip-RAM snapshot. Instrument-only events count;
all stored patterns are scanned conservatively so pattern selection is safe.
Referenced samples have independent Chip allocations, pinned until replay stops. Sample numbers,
pattern rows and source/master data are preserved. Empty and omitted samples
receive safe empty headers and the existing owned silent DMA word.

The immutable full export and live-edit comparison workspace use bounded Fast
RAM when available. Live row edits among cached instruments remain supported.
A changed instrument working set, sample header or PCM stops replay and releases
the cache before a subsequent play rebuilds it. Comparison uses the immutable
export, so EFx mutations of private Chip data do not corrupt the master or create
false invalidations. This intentionally stops even on an unused master change.

Pattern/header storage now uses bounded Fast RAM when available. The native
replay adapter takes an explicit 31-entry Chip sample pointer table; the pinned
engine still performs its original loop and first-word initialization on those
private sample copies. Empty slots share a separate owned silent Chip word.
The scope renderer bounds reads against owned sample buffers, not Fast metadata.
All partial allocations are released on failure; replay is stopped before any
buffer is freed.

`sample_cache` provides a bounded versioned resource pool. A successful acquire
pins a resource; newly loaded or converted data stays unpublished until the
caller confirms completion. Invalidation retires pinned versions without freeing
voice-owned memory; the final release discards them. Unpinned allocations are
evicted in least-recently-acquired order under budget or backend allocation
pressure. Stale handles cannot release a newly assigned slot. Retired leases
cannot republish invalidated content. Clear refuses to claim complete disposal
while a voice still holds a pin. Calls belong on the owning control thread, not
inside an audio interrupt.

Paula uses that pool for its independent Chip sample blocks. Direct Play/Pattern
restarts stop voices, unpin the previous set and reuse matching slot allocations
when sizes agree. Every restart uses a new generation and reloads all referenced
PCM from the immutable export before the replay's first-word fixes. This avoids
carrying EFx mutation or a previous project's contents into a new session. Unused
old blocks can be evicted during allocation. Explicit Stop, invalidation and any
failed start still release the entire pool; there is no idle Chip-RAM retention
across an explicit Stop. Long-lived live voices remain pinned.

Song playback still requires a classic-compatible four-channel project.
Sample audition can now derive an 8-bit Paula representation of mono 8/16/24-bit
masters, including filtered conversion of non-looping samples to the classic rate. The source stays unchanged; an odd final byte is
padded with silence for DMA, without changing the master's frame count. The
existing classic length and loop restrictions still apply. Enhanced song dispatch remains unsupported. AmiGUS cache eviction and
Studio streaming remain incomplete. Native render/stem helper allocations and
remaining generic utility buffers also need their own allocation audit.

## Persistence and evidence

Enhanced project saving serializes project/master PCM and metadata. Classic MOD
export uses explicit conversion rules and must not alter that master. Backend
caches must never be serialized as replacement master data.

`tests/test_master_memory.py` compiles the production allocator with a mocked
Exec API and ASan/UBSan. It checks queried capacity, shared accounting, allocation
failure, live memory pressure, installed-but-exhausted Fast RAM, no Chip spill,
Chip-only reserve, exact release sizes and overflow rejection. This is host
policy evidence. Cross-compilation is not emulator or physical acceptance;
real memory-placement and playback-pressure checks remain required.

`tests/test_paula_cache.py` checks selective payload packing, instrument-only and
inactive-pattern references, source preservation, capacity/alias/malformed input
refusal, and live-edit invalidation under ASan/UBSan. `PTPaulaTest` additionally
contains native assertions for memory placement, unused payload omission and
stopped rebuild after new instruments or changed masters. Cross-compiling these
assertions does not mean they have run; consult the current checkpoint for guest
evidence.

`PTSampleCacheTest`/`tests/test_sample_cache.py` exercise publication, pinned old
versions coexisting with new revisions, cancellation, LRU eviction, resource
shortage below the nominal budget, stale handles, overflow and complete release.
The same portable core is intended for other playback backends; an AmiGUS driver
must still supply and validate card allocation/upload/release and generation
rules. A passing resource-pool test is not AmiGUS implementation or acceptance.

## Derived PCM representations

`playback_pcm` packs a selected master channel into signed 8- or 16-bit bytes.
16-bit byte order and even-byte padding are explicit settings. Reduction rounds
nearest with ties away from zero, then clips to the target range; there is no
implicit dither, resampling, downmix or loop relocation. The master data and its
format/metadata are never overwritten. Empty hardware slots use backend-owned
silence guards rather than zero-byte cache allocations.

The cache helper keys each identity by format/channel/order/padding and requires
a changed revision for every master or relevant metadata edit, including undo.
Different representations can coexist. Invalidation retires every representation
of that identity while active leases remain pinned. Use separate pools for each
backend/memory domain: a Chip allocation is not a card-RAM upload.

Paula audition uses bounded Fast workspace to prepare its derived 8-bit sample,
then releases that workspace after the owned playback snapshot is built. Its
master remains 16/24-bit for saving, editing and Studio use. The 8/16-bit packer
and cache helper are available for an AmiGUS adapter, but card allocation/upload
and live Studio streaming remain separate unfinished work.

## Device upload boundary

`pt_playback_pcm_upload` now uses a dedicated device-backed cache pool whose
allocator returns opaque resource descriptors, not CPU-writable sample pointers.
The caller supplies bounded Fast-RAM staging. Conversion reads the master;
a synchronous driver callback transfers the bytes. Only a completed transfer is
published. Failed or partial transfers release unpublished device resources and
leave the output lease unchanged. Cache hits skip staging and upload. Capacity
or conversion failures may follow eviction, but never discard master samples.

A successful lease must stay pinned until all device voices using it have
stopped. Master invalidation retires old pinned resources; they cannot be reused
for new triggers and are freed on final release. Clear cannot free active leases.
The driver must account for actual device allocation constraints and provide a
stable non-NULL descriptor even if the card address is zero. Each pool belongs
to one backend/session; it must be cleared and all voices released before its
driver context is destroyed. Callbacks are synchronous and must not reenter the
cache or mutate the master. Asynchronous DMA upload needs a separate completion
protocol and is not enabled by this interface.

The pinned `amigus_lib.sfd` provides discovery, reservation and interrupt calls;
it does **not** provide sample allocation/upload functions. No invented library
vectors or hardware registers have been added. This implemented driver boundary
and its fake-device tests are preparation for AmiGUS integration, not a working
AmiGUS allocator, uploader, voice dispatcher or physical acceptance. The native
editor does not call this device upload path yet.

`tests/playback_upload_test.c` verifies descriptor-backed transfers, 24-bit master
preservation, cache hits without transfer, format coexistence, pinned invalidation,
shortage refusal, partial-transfer failure/retry, staging capacity and alias
refusal. Host tests use ASan/UBSan; cross-building does not mean execution on the
emulator or a card.

### Bounded upload staging

`pt_playback_pcm_upload_chunks` removes the requirement for a full-sample staging
buffer. The caller can provide a small Fast-RAM buffer (minimum one output frame:
one byte for 8-bit, two for 16-bit). The converter validates the complete master
once, then emits ordered chunks without allocation or repeated full-source
validation. Each 16-bit frame stays intact; an odd staging capacity leaves its
last byte unused for 16-bit output. Optional 8-bit DMA padding appears only at the
end of the entire representation, including when it requires its own chunk.

The device writer receives byte offsets, a borrowed buffer and byte count. It
must finish consuming each chunk before returning nonzero; the buffer is reused
immediately. The resource stays unpublished until every write completes. Any
failure releases the entire partial resource, keeps the caller's output lease
unchanged and allows a clean retry from offset zero. Existing cache hits need no
staging. The driver remains responsible for hardware-specific transfer alignment,
address constraints and completion; these generic byte writes do not assert an
AmiGUS transfer protocol. Calls and source ownership remain single-threaded.

The upload test sweeps 8/16-bit, both source channels, both byte orders, padding
settings and buffer sizes from one frame to eleven bytes. It compares every
assembled device image against whole-sample conversion, checks staging canaries,
and injects a failure on the second chunk followed by a full retry.

### Filtered mono Paula preview

`paula_preview.h` plans the derived frame count before allocation and refuses
output longer than the classic131070-byte sample limit. The native audition path
allocates only the planned int32 workspace from its bounded Fast-first allocator.
For a changed rate it applies the existing integer antialias filter at source
precision, then rounds/clips that workspace to8-bit. An odd last frame gets a
silent DMA pad. Same-rate previews use direct precision conversion. The source
buffer must not overlap the output; original PCM metadata and data stay intact.
The replay owns its separate Chip copy before this workspace is released.

Forward loops use the derived-only mapping policy below.
Slice playback and enhanced song routing are unchanged. Filtering is synchronous with progress/cancellation callbacks; target latency
and performance still require separate validation.

### Forward-loop preview coordinates

Forward-loop start/end are scaled from master rate to preview rate, then rounded
to the nearest two-frame Paula DMA boundary; exact ties advance. The end is
clamped to the last complete word of real converted audio, excluding any silent
padding. Collapsed loops and loops of only one word are refused explicitly.
This applies to same-rate odd endpoints as well as rate conversion. All mapping
is performed on the local playback sample; stored loop points never change.

The conversion filter still uses the source endpoint extension used by existing
offline conversion. It is not loop-aware filtering or a promise of seamless
loop audio. Ping-pong/crossfade loops remain unsupported in this Paula bridge.
Listening and physical performance validation remain separate requirements.

### Stereo Paula audition

Stereo8/16/24-bit masters now retain both channels in sample preview. Conversion
and antialias filtering use interleaved Fast workspace; the derived8-bit values
are split into two mono playback samples. Both trigger on the same row using
classic Paula voices0 (left) and1 (right), with the same period, volume, finetune
and mapped forward-loop coordinates. There is no downmix or change to the master.
Odd-frame silence padding is applied independently to both channels.

Stereo workspace is bounded to four int32 elements per padded output frame:
two interleaved conversion values plus separate left/right planes. The existing
per-channel131070-frame limit and current Fast-memory budget apply before use.
Each channel gets its own owned Chip playback copy; allocation failure follows
the existing replay cleanup path. The Fast workspace is freed after replay owns
its snapshot. This is sample audition only, not mixed16-channel song routing.
Pinning, explicit Stop and no-stealing backend ownership remain unchanged.

### Preview cancellation

`pt_paula_preview_prepare_progress` forwards progress to the existing bounded
filter callback, with cancellation before conversion and before successful
completion. Same-rate conversion polls at entry/completion; it does not claim
interruptible per-sample precision conversion. Validation scans and the final
stereo split also remain synchronous. Callbacks must not edit the master or
reenter playback APIs. Partial staging is disposable, never a valid playback copy.

`pt_paula_audition_progress` releases workspace on cancellation and reports it
without starting new replay or replacing currently active playback. The original
non-cancellable API remains as a wrapper. The native editor connects the existing
modal conversion UI: Escape cancels, refresh events are handled, editing input is
not applied during conversion. This reuses the existing status/progress display;
there is no layout change. Full interactive Escape/refresh acceptance and real
A1200 latency remain distinct from callback-level tests and cross-compilation.

## Repeatable native memory suite

The native build now registers `PTPlaybackUploadTest` and `PTPreviewPCMTest`.
`tools/shared_infra_paula.py` runs them with the existing cache, PCM and Paula
regressions after shared emulator ownership/target checks. Every binary has its
own return code, log and hash; upload/preview assertion markers are required.
The first five-binary emulator pass is recorded in
`evidence/enhanced-editor/native-memory-suite/`. Upload resources in that test
are simulated descriptors, not AmiGUS RAM. This closes portable-core execution
coverage on68030, not driver/hardware acceptance or interactive UI qualification.

## Interactive cancellation evidence

`evidence/enhanced-editor/interactive-preview-cancel/` records keyboard Escape
cancellation, unchanged project bytes after saving, stopped DMA and normal exit
on the shared030 emulator. The repeatable fixture uses stereo24-bit192000Hz PCM.
The runner isolates recent-file storage and verifies restoration of its prior
ENV override. The initial run's default-recents side effect is documented.
Captured display artefacts are unresolved; this is functional cancellation
acceptance, not clean visual/refresh acceptance or a release candidate.

A settled A/B run in `evidence/enhanced-editor/display-isolation/` compares the
current uncommitted display build to an isolated351a8ca build. Lower sampler/footer
corruption is absent in the committed build while functional cancellation and
project-byte identity pass in both. The uncommitted display changes remain
preserved and unqualified. Long cancellation/status text is crowded in both;
visual readability and deliberate refresh testing remain open.

The compact preview messages are qualified in
`evidence/enhanced-editor/status-fit/`: cancellation fits the existing single-line
status area on a committed-source candidate. No master or layout behavior changed.
Progress wording fits the same character bound but was not separately captured.
Uncommitted display work and deliberate refresh-event acceptance remain open.

## Render/stem verification buffers

The offline mixer already reads immutable master PCM directly, retaining true
24-bit values for 24-bit WAV output. Render and stem file verification now uses
bounded descriptor reads instead of stdio read-ahead buffers. Each comparison
uses at most1536 bytes; encoding uses a separate1536-byte block and
the mixer emits at most256 stereo frames. Short reads and EINTR are handled;
truncated, corrupted or trailing bytes prevent publication. Master PCM is never
replaced by these output blocks. Stem export reuses the same verification path.

This removes the verification stream's implicit stdio buffer, not all library
allocation. File descriptors/runtime internals and remaining stack placement still require
native memory/performance qualification. The allocated render and WAV workspaces
are described below; live24-bit Studio transport to AmiGUS remains open.

## Allocator-backed reference renderer

The native editor now routes render preflight, WAV render/verification and all
stem passes through its shared bounded master-memory allocator. Timeline/pitch,
voice state and the512-element int32 PCM block use one allocation per stream
call, released on success, cancellation, sink failure or preflight refusal.
Measurement uses one smaller timeline allocation. On Fast-equipped systems the
existing allocator refuses Fast exhaustion rather than spilling into Chip RAM;
`PT_RENDER_MEMORY` is reported as RENDER: OUT OF MEMORY. No master edits occur.

Portable allocated APIs require an allocator; original stack-based APIs remain
available. File/stem allocated entry points accept NULL for legacy behavior.
Source lifetime/immutability and callback non-reentrancy obligations remain.
Sequential measure/render/verify calls allocate and release their workspace
independently; memory can become unavailable between passes, in which case
owned file staging is removed and the old destination/master is preserved.

Small helper locals still use stack space;
this does not qualify every stack byte or runtime file-descriptor allocation.
Bounce also uses the allocated measurement/stream path described below.
Live Studio streaming is not implemented by these offline APIs.

Pinned m68k `-Os -fstack-usage` inspection reports the allocated measurement and
stream entry frames as64 and72 bytes, versus2364 and5532 for legacy wrappers.
Their shared measure/stream helper frames are336/488 bytes. These are compiler
per-function figures, not whole-call-stack peaks or physical runtime proof.

## Bounded sample-bounce workspace

Sample bounce now uses the sampler allocator for both measurement and mixer
workspace. Native sampler allocations already share the bounded Fast-first
master pool, so the output version, undo resources and temporary mixer compete
within that pool. Temporary workspace is released before the new sample/history
entry is committed. It is not retained as undo data or counted in persistent
sampler-history bytes; the allocator's shared ceiling still applies.

Allocation failure during measurement or after staging the output PCM reports
`PT_RENDER_MEMORY`/`PT_EDIT_CAPACITY`. The existing append transaction discards
only its own provisional resources. Master samples, slot table, report and
existing undo/redo remain unchanged. Tests inject failure at every allocation
in a fresh bounce and with existing redo, then successfully replay the preserved
redo. Cancellation and exact24-bit project round trips remain covered.

## Export memory-pressure rollback

The allocation-failure regression covers all four WAV workspace allocations
(file, measure, render, verification) and all eleven for a two-stem export (batch workspace, two global
preflights, then file/measure/render/verification for each stem). Every refusal must
report `PT_RENDER_MEMORY`, release working allocations, preserve the output
report and all master/project bytes, and remove every owned staging candidate.
Failures in the second stem also remove the already verified first staged stem.
This tests the allocated export path, not real Fast-RAM fragmentation or filesystem
hardware failure. Existing destination/race and cancellation tests remain separate.

## Native WAV file workspace

Allocated WAV export now owns its path/state,44-byte WAV header,1536-byte encoded
PCM block and separate1536-byte verification block in one caller allocation.
Native WAV and each stem's WAV therefore use the same bounded Fast master pool
as the mixer. The file block remains pinned for the synchronous export while a
measurement/mixer block is temporarily live. Both compete with masters/undo;
no fallback to Chip occurs on Fast-equipped systems. Failure cleans owned files
before releasing file storage. Source master PCM remains untouched.

The portable legacy WAV entry retains a stack workspace; NULL allocator retains
that behavior. Runtime library internals
still need a separate audit. This change does not claim all stack use eliminated
or qualify live Studio/card transport.

## Native stem batch workspace

Allocated stem export now owns its batch report,16 measurement reports, working
render options and staging/file path arrays in one bounded block. This remains
live while one WAV file block and one mixer/measurement block are nested, for a
maximum of three simultaneous workspace allocations. The native editor supplies
the same shared Fast-first allocator to all three. Each is released after its
own cleanup; failure in a later stem removes already completed staged stems
before the batch workspace is freed. Legacy entry and NULL allocator retain
stack-based behavior. Master/source data remain immutable through all passes.

All eleven allocation refusals for two stems are tested, including initial batch
allocation. WAV still has four failure points. Small helper locals and runtime
file/library allocations remain outside this explicit workspace policy; actual
hardware memory pressure/performance and live Studio output remain unqualified.

## Exec-backed emulator memory fixtures

Native-only wrappers reuse the bounce, allocated-stem and export-failure fixtures
with the production `master_memory.h` allocator. Every explicit fixture allocation
is checked using `TypeOfMem`: Fast must be set and Chip absent. The pool starts
from current Exec memory queries; completion requires owned-byte accounting back
to zero. A zero pool ceiling briefly tests real allocator budget refusal without
exhausting the shared emulator. Existing injected failures remain deterministic.

These wrappers qualify explicit document/sampler/export workspace allocations on
the shared030 guest. They do not instrument C-library internal allocations, prove
physical A1200 fragmentation behavior or measure whole-machine memory recovery.
Fixture source arrays may be on the stack; no claim that every fixture byte is
Fast RAM. Physical performance/listening and AmiGUS remain separate acceptance.

## Project and sample save workspace

Native PTG/MOD/WAV/raw/IFF saves now supply the shared bounded allocator for file
paths/state and the4096-byte verification buffer. Descriptor reads replace stdio
read-ahead and handle short reads/EINTR; expected bytes, exact EOF and no-replace
publication remain required. Workspace refusal returns `PT_SAVE_MEMORY`, shown
as SAVE: OUT OF MEMORY, with edits/destination preserved. Legacy callers retain
a stack workspace. The file runtime itself is not claimed allocation-free.

See `MEMORY_AUDIT.md` for remaining import/recent-file/runtime/display boundaries.

Project/sample imports now read directly into the bounded master-pool payload
through descriptors, with a64MiB ceiling and no FILE/read-ahead buffer. The helper
returns ownership only after complete reads, exact EOF and successful close;
allocation failure or changing length releases the temporary data before decode.
Short reads and EINTR are handled. Same-size concurrent modification is not
detected, and runtime descriptor allocation remains outside this accounting.

Recent-files persistence now accepts a required allocator in its allocated APIs;
the native editor supplies the shared master pool for its two lists, record and
path workspace. Every exit releases this workspace. Descriptor I/O retries short
transfers/EINTR; alternating generations and verification retain the last valid
list on an interrupted write. Allocation refusal leaves caller/persisted data
unchanged. Legacy APIs retain heap allocation; no whole-runtime claim is made.

Shared030 import and recent-memory behavior fixtures now pass individually with
owned cleanup. Concurrent import shrink/growth remains host-only: AmigaDOS denies
the simultaneous write reopen. These two fixtures use tracking heap allocators;
Exec-backed Fast-RAM pointer accounting for them remains a separate next check.

Exec-backed variants of the import/recent fixtures now pass on shared030 using
the production master allocator: 2 and534 explicit Fast/not-Chip allocations,
respectively, with zero pool-owned bytes at exit and zero-budget refusal without
Chip fallback. Runtime-library allocation and physical pressure remain unmeasured.

## Incremental Studio voice ownership core

`studio_mix` adds an allocator-owned session of up to16 voices. A caller-provided
master-version acquire/release interface pins immutable source PCM for each voice.
The session copies only PCM descriptors, reads master samples directly and pulls
1..256 stereo24 frames at48kHz into caller storage, preserving voice phase across
blocks. New triggers pin/validate first; failure preserves the old voice. Stop,
replacement, natural completion and session close release exact pins.

This is a control-thread mixer building block, not an interrupt handler or a
finished Studio mode. The source provider must keep pinned revisions immutable
and alive; integration with document/sampler version ownership remains required.
Tracker tick scheduling, output queue/device transport, underrun recovery and
empirical 68030 capacity remain unimplemented here. Sixteen slots are a bound,
not a claim of16 real-time voices. There is no allocation/acquire in block reads,
but completed voices call release; do not invoke from an audio interrupt.

## Sampler-to-Studio master pins

`pt_sampler_pin/unpin` now retains the sampler's immutable version independently
of current-slot and undo-journal ownership. The Studio provider maps slot+1 and
sampler generation to a pin; stale generations refuse new triggers. Existing
voices retain their exact version across edits, undo/redo and history eviction.

For a document-backed sample without an owned sampler version, the first pin
promotes PCM/metadata into a bounded sampler allocation and updates the project
slot to that authoritative version. Precision/content and undo state are unchanged;
the original document allocation lives until document release. This temporary
duplication is budgeted and can fail gracefully; subsequent pins share the owned
version. It is not a degraded playback cache.

Pins can outlive release of current/history ownership, but the sampler structure
and allocator context must remain alive and must not be reinitialized until final
unpin. Normal integration must close mixer sessions before editor/document reset.
The provider is implemented and lifecycle-tested; native editor scheduling and
AmiGUS output remain unwired. No real-time performance acceptance is implied.

Production-allocator Studio ownership fixtures now pass individually on shared030:
mixer1 and sampler11 explicit Fast/not-Chip allocations, each with zero pool-owned
bytes at final release. This verifies pin/ref lifetime with Exec allocation, not
real-time deadlines or AmiGUS transport. No physical acceptance is implied.

`pt_studio_control` applies a selected-channel batch of positive Q32 pitch steps
and bounded Q16 left/right gains between blocks. It validates the whole batch
before writing; invalid controls, out-of-range channels or inactive selected
voices leave all voices unchanged. It preserves phase, loops and pins, performs
no callbacks or allocation, and lets zero-gain voices continue advancing. This
is the control primitive for future tracker ticks, not a completed scheduler.

## Bounded tick intervals

`studio_tick` borrows a mixer and uses the existing48kHz ideal-BPM Q32 frame clock
to reserve one tick interval within an explicit session frame budget. Reads consume
1..256 frames without crossing the pending boundary. A new interval is refused
until the current one drains; invalid BPM, exhausted budget and failed reads do
not advance timing/remaining state. Fractional frames carry across tempo changes.

This is an interval reader, not automatic tracker dispatch: the caller still
applies commands at drained boundaries and supplies the next interval's tempo.
It adds one bounded allocation; closing it does not close the borrowed mixer.
Do not bypass it by reading the mixer directly while an interval is pending.
CIA timing, effect-to-voice mapping, device output buffering and deadline/underrun
handling remain separate work. The native editor is not yet wired to this API.

## Pinned repeat-source handoff

The renderer audit identified instrument-only/delayed-note repeat-source changes
as a prerequisite for faithful tracker dispatch. `pt_studio_repeat` now pins a
second master version and schedules its bounded forward repeat without restarting
the current segment. Format/channels/rate must match. Pending replacement is
transactional; failure preserves current/pending voices and pins. After a successful
block crosses the boundary, the mixer transfers descriptor ownership and releases
the old pin; no PCM copy or conversion occurs. Stop/retrigger/close release both
current and pending pins. Pending data participates in output-alias checks.

This closes a mixer capability gap; it does not yet wire tracker events or the
AmiGUS transport. Independent initial/repeat segment triggers and complete audited
effect dispatch still need integration before enhanced song playback is enabled.

`pt_studio_trigger_segment` now pins one master for independent initial and
forward-repeat ranges, reusing `pt_voice_init_segment`. Disjoint ranges and
loop-aware interpolation preserve the voice engine's behavior. Both ordinary
and segment triggers share staged acquisition/validation and release old/current
plus pending pins only on success. Invalid segment or loop kind leaves playback
and pending handoff unchanged. This supplies the range primitive needed by the
renderer’s offset/retrigger/delay mapping; tracker dispatch remains unwired.

## Shared effect-command state

The reference renderer now owns a `pt_render_command_state` containing voices,
gains, instrument/volume/velocity and tremolo memory. Shared low-level init,
completed-tick and gain functions reuse the existing effect body unchanged; the
reference stream calls these functions directly. This exposes audited state for
the upcoming Studio command adapter without introducing a second effect engine.

The API requires renderer-preflighted, consistent immutable inputs. Its PCM
pointers are borrowed, not pins; it must not be used as a substitute for Studio
master ownership. A failed command tick may leave partial state and must abort
the session. Command-intent translation into Studio triggers/controls and full
tracker playback are still unfinished. This refactor alone adds no live output.

## Explicit playback command plans

`pt_render_commands_plan` now records the same completed-tick effect execution as
an ordered, caller-owned plan: ordinary triggers, independent segment triggers,
repeat-source changes, stops and final step/gain controls. A fixed 64-entry bound
covers sixteen channels without allocating. Repeated notes remain explicit even
when the new phase equals the old phase. The reference stream retains its ordinary
unrecorded path through the same effect implementation.

The plan borrows PCM descriptors; it does not acquire master versions. Dispatch
must resolve each source to its master key/generation and acquire through Studio's
provider. Inputs still require renderer preflight and consistent immutable
lifetimes. On failure the plan count is cleared, but command state may already
have advanced: abort the session, never dispatch or retry a partial tick.

Host tests replay the operations through voice APIs and compare 16-channel audio
with the reference command path across repeated notes, instrument-only handoffs,
volume cuts, offset segments, retriggers and stops. The Studio translation,
complete song scheduling and hardware output transport remain unfinished.

## Studio command dispatch

`pt_studio_dispatch` translates a successful explicit command plan into Studio
trigger/segment/repeat/stop/control calls. The owner supplies immutable descriptor
identity to master-key/version bindings. Unknown or duplicate source bindings and
invalid channels are rejected before provider callbacks. The provider must return
the exact master content/format associated with the binding; no pointer arithmetic,
PCM conversion or playback-cache substitution is used by dispatch.

A later acquisition/control failure is fail-stop: every voice in the dedicated
session is stopped, releasing current and pending pins. This does not roll back
command state; the caller must discard it and abort playback. Dispatch itself
allocates nothing, although provider acquisition may promote/pin a master.

Host tests compare true24 output against the reference effect path across sixteen
channels, including handoffs, cuts, segments, retriggers and stops, and verify
cleanup after partial acquisition failure and ambiguous source bindings.
`PTStudioPlanTest` is included in the native cross-build. Full song scheduling,
command-state phase synchronization, editor integration and output transport are
still pending; the adapter alone does not enable live Studio song playback.

## Command-state phase synchronization primitive

`pt_voice_advance` advances up to16 initialized/zeroed voices by up to256 frames
using the exact phase transition shared with the audio reader. It reads no PCM,
allocates nothing and performs no callbacks. This lets a caller advance borrowed
command-state mirrors only AFTER a successful Studio block, without generating a
second discarded audio mix. It preserves loop, pending-source handoff and one-shot
end behavior, including fractional and very large steps. Invalid count/frame
arguments leave state unchanged.

The Studio dispatch test now advances its command-state mirror this way and still
matches reference 16-channel true24 output. Separate tests compare complete voice
state against repeated audio reads for forward/pingpong/one-shot/segment/handoff
cases. This primitive does not itself schedule songs or own borrowed descriptors:
the caller must still preserve immutable source lifetimes, advance by precisely
the successful output frame count, and abort on dispatch/read failure. Full song
scheduling, editor integration and live output remain unfinished.

## Incremental audited sequence

The allocator-owned `pt_render_sequence` reuses renderer measurement, timeline,
pitch, range handling and explicit command plans. Open performs full bounded
preflight/measurement before exposing the session, using one allocation. The
project and all source storage remain borrowed and immutable until close.

Each next interval describes preceding audio frames, whether to emit or discard
pre-roll, and whether it is the ending interval. The caller reads Studio in blocks
of at most256 frames, consumes only successful reads to advance the command mirror,
then completes the interval and dispatches its plan before requesting another.
Zero-frame intervals also require completion. Protocol mistakes are refused;
internal sequencing/command failures poison the session. Closing frees sequence
state only; the owner must stop/close Studio and discard partial output on failure.

The host integration test drives a complete bounded song through Studio and
compares every emitted true24 value to reference rendering for block sizes1/17/256,
with tempo changes, pattern delay, retriggered notes, volume slides, initial lead-in
and row-range pre-roll. Allocation/preflight/protocol refusal is also covered.
This establishes a caller-driven song path; it is not yet an editor playback
controller, output queue, AmiGUS transport or real-time performance qualification.

## Owned Studio song controller

`pt_studio_song` owns the incremental sequence, mixer, copied master bindings,
command-plan workspace and stereo24 output scratch. It uses three bounded caller
allocations; each failed startup stage releases earlier allocations and leaves
the output session pointer unchanged. Input is restricted to48k stereo24 output.
One binding per sample must match that project slot's descriptor. The project and
master contents remain immutable/borrowed until close; stop before editing or
replacing the document. Provider pins do not replace that project lifetime rule.

Each pull performs at most one state transition or one block of up to256 frames,
including discarded pre-roll. A null block with done=false means progress; the
owner should yield/check cancellation before pulling again. Output is borrowed
session scratch and must be copied before another pull. No output queue is hidden
inside the controller. Natural end, stop and internal failure close the mixer and
sequence and release all pins; the controller allocation remains until
close. Failure is sticky and playback cannot be retried. Stop is idempotent.

Host tests cover reference audio at block sizes1/17/256, all three allocation
failure points, stop during a pinned voice, natural cleanup and sticky acquisition
failure. This provides an owner-thread playback controller, not an editor action,
interrupt callback, device queue, transport or real-time performance qualification.

The production-allocator song fixture now passes on the coordinated shared030
emulator:18 tracked allocations were confirmed Fast/not-Chip, startup budget
refusal did not fall back to Chip, and final owned bytes were zero. The guest
exercises256-frame song reads across normal/lead-in/row-range cases, allocation
failure stages, stop and acquisition failure. Host coverage retains1/17/256.
This qualifies tracked controller/sequence/mixer allocations, not CRT/static
buffers, hardware audio, physical A1200 performance or AmiGUS memory/transport.

## Sampler-owned song bridge

`pt_sampler_song` builds slot+1 bindings using the sampler's current generation
and keeps its provider context alive for the entire core song session. Initial
sample promotion uses the sampler's existing budgeted immutable master mechanism,
preserving24-bit values. Natural end/failure closes core song state; explicit stop
is idempotent, and close releases the bridge allocation.

The owner MUST stop before edits/undo, document replacement or sampler/history
release/reinitialization. The bridge additionally detects generation or project
sample/event/order table replacement before each pull and refuses stale playback,
releasing core pins. This is defensive refusal, not support for concurrent edits
or destruction of owner objects. In-place pattern changes are not detected and
still require explicit stop. The native editor actions are not yet wired here.

Host coverage verifies initial promotion, true24 values, stop/edit/restart with
new generation, stale-generation refusal and stop-before-owner-release cleanup.
A bridge stopped before owner release can then be safely closed. This component
adds no hardware output or physical-performance claim.

## Editor stop-before-change guard points

The editor now exposes a synchronous owner callback through
`pt_editor_change_guard` / `pt_editor_prepare_change`. Pattern edits, channel/title
changes, undo/redo, song-order mutations and sampler edits invoke it before their
mutation call, including refused attempts. Disposal invokes it before releasing
history or sample owners. Ordinary navigation does not stop through this hook.

Native imports and sample bounce, Stop, new-song and document-load paths now invoke
the same guard before mutation/replacement. Owners must install an idempotent
session-closing callback and reinstall after editor initialization; initialization
clears the hook. It must not reenter mutation. External callers bypassing these
entry points remain responsible for calling prepare_change before modifying data.

The callback is an integration boundary, not an enabled Studio backend. No native
Studio session or device output is started by it. Host checks verify callback
ordering against pre-edit note data, undo and disposal, and navigation preservation.

## Editor Studio session owner

`pt_editor_studio` is a small caller-owned attachment that installs the editor's
change guard and closes its `pt_sampler_song` synchronously when the guard fires.
It refuses an existing guard, starts with the editor sampler allocator, and closes
completed/failed playback. Stop and detach are idempotent; detach removes only its
own hook. Detach before freeing or reinitializing editor memory. Editor disposal
may run first because its guard stops playback before releasing sample owners.

The owner exposes explicit start/pull calls for a future output driver; it does
not change native PLAY behavior or create a device queue. The native application
has not instantiated it yet. Tests exercise actual pinned song playback through
note edits, undo and disposal, prove navigation leaves the session running, and
verify guard ownership plus zero remaining tracked allocations at final cleanup.

The production-allocator editor/Studio fixture now passes on shared030. Its21
tracked allocations (including the test editor object and callback-allocated
project/sample/song state) were Fast/not-Chip, with zero final owned bytes and
budget-refusal behavior preserved. Pinned playback stopped on note edit, undo and
disposal; navigation preserved it. This is core editor/controller execution, not
native UI interaction, live device audio, physical A1200 performance or AmiGUS.

## Bounded Studio output queue

`pt_studio_queue` provides1..8 copied blocks, each at most256 stereo24 frames at48k,
inside one caller-allocator allocation. Push preserves source/master data and all
24 bits; full queues refuse without overwriting queued or consumer-held audio.
A serialized consumer acquires one read-only block lease and releases it with a
monotonic ticket, preventing stale releases after slot reuse. Queue/data lifetimes
must cover any device reference; release only after completion or confirmed cancel.

Finish closes production and drains queued blocks. Abort discards unleased blocks
but retains an outstanding lease; close refuses while it is held. Neither operation
cancels a device transfer. Operations are owner-thread serialized, not lock-free,
interrupt-safe or DMA-memory qualification. Ticket exhaustion refuses new leases.

Host tests cover exact24-bit extrema/low bits, source-buffer independence, repeated
ring wrap, full/backpressure, wrong/stale release, busy close, finish/drain and
abort-held-block cleanup. Producer/song integration and device consumers are still
unfinished; the queue alone does not make live AmiGUS output available.

## Bounded producer pump

`pt_studio_pump` connects a pull/stop producer (including `pt_studio_song`) to the
output queue. Caller-owned state includes one private pending256-frame block;
there are no pump allocations. Each step makes at most one producer pull or one
pending retry. Full queues preserve the pending copy and prevent any further song
advancement until it is enqueued. Source scratch can therefore change only after
its data is safe. Producer end finishes the queue for draining, then stops source
ownership; stop/failure abort unleased queue audio while preserving held leases.

Producer/queue contexts must outlive the pump. Operations remain serialized owner
thread calls, with no device cancellation or concurrent/interrupt guarantee. A
consumer must explicitly release its device-held lease before queue destruction.
The pump does not create a native PLAY route or claim an available AmiGUS device.

Tests verify exact ordered24-bit frames under repeated stalls and held leases,
provider failure with a lease held, and full queued-song output against reference
at block sizes1/17/256 including lead-in and pre-roll. No dropped/duplicated frames
were observed in those deterministic cases. Hardware deadlines remain unqualified.

Queued-song and pump fixtures now pass under production allocation on shared030:
21 and4 tracked Fast/not-Chip allocations respectively, zero final owned bytes in
both, with budget refusal retained. Queued song guest coverage uses256-frame
blocks across normal/lead-in/pre-roll; host retains1/17/256. The native pump tests
stalls and failure with a held lease. This is not device DMA, timing, physical
A1200 or AmiGUS acceptance; consumer integration remains unfinished.

## Editor ownership of queued Studio audio

The optional `editor_studio` owner can now drive its sampler song through the
bounded pump into a borrowed output queue. Editing, undo, editor disposal and
explicit Stop close source pins, discard pending output and abort unleased queue
blocks. A consumer-held block remains valid until the consumer releases it;
queue close continues to refuse while leased. Natural song completion instead
finishes production and drains the queue. Stop/detach must precede queue close,
even after natural completion. Direct pull is refused while queued.

This is a tested editor ownership API, not a wired native PLAY/device consumer.
All calls remain serialized; Stop does not cancel a physical device transfer.

Queued editor ownership also passes on shared030 with42 tracked Fast allocations,
zero final owned bytes and budget refusal. Host editor/Studio suites pass. This
still does not qualify a native PLAY route, device cancellation or hardware audio.

## Output-consumer completion ownership

`studio_consumer` is an optional caller-owned, serialized transport adapter. It
borrows a queue and bounded submit/poll/cancel callbacks. Temporary submit refusal
retains the unsent lease for retry, without dropping audio. Accepted buffers stay
leased until poll or cancel explicitly confirms all transport references are gone.
Pending or failed cancellation retains memory; even a sticky error permits polling
for eventual safe release. Stop aborts unleased audio. Detach refuses until the
held lease is safe. Stop the producer separately and retain queue/context lifetime
until successful detach. This does not itself install a device backend.

Host sanitizer tests cover busy retry, natural drain, submit failure, successful
cancel, delayed/failed cancel, polling failure and eventual completion. Pinned
Amiga compilation passes; this new consumer has not yet run in the emulator or on
hardware. Callback compliance, native transport, DMA and deadlines remain open.

The queued-song integration fixture now passes audio through the actual consumer
adapter and a deterministic delayed transport: busy submissions and three-poll
completion delays preserve exact reference frames at host block sizes1/17/256,
including lead-in and pre-roll. A stopped real song releases source pins while
failed cancellation retains its independent output copy until confirmation.
The native queued fixture uses256-frame normal/lead-in/pre-roll plus a17-frame
cancellation case; static fixture storage is not part of tracked allocation proof.

Shared030 consumer and queued-chain fixtures pass with6 and25 tracked Fast/not-Chip
allocations, zero final owned bytes and budget refusal. This supersedes the earlier
consumer emulator gap only; device transport and physical acceptance remain open.

## AmiGUS PCM packing preparation

Pinned AHI source exposes a separate stereo24 PCM FIFO path. The new pure
`amigus_pcm_pack` preserves true24 samples as numeric MSB-first FIFO words,
carrying odd frames across blocks; explicit finish reports at most one silent
padding frame. No master degradation or device access. Host sanitizer and native
compile checks pass; positive-card/runtime transport is still unfinished.
See [transport review](AMIGUS_STUDIO_TRANSPORT.md) for pinned evidence and gaps.

The staged `amigus_fifo` layer copies packed words into bounded caller-owned
storage. Capacity stalls retain them, uncertain writes require confirmed reset,
and reset clears odd-frame carry. Poll completion concerns host-buffer lifetime,
not device silence. Host and compile checks pass; native MMIO remains unwired.

The queue/consumer/staged-FIFO chain now passes host and shared030 fake-port
checks at1/17/256-frame partitions, including failed-reset lease retention.
Four tracked Fast allocations, zero final owned bytes; no hardware audio claim.
Session stop must reset device FIFO separately even with no outstanding lease.

`amigus_session` now explicitly coordinates final tail, device drain acknowledgement
and confirmed disable/reset before detach. Stop resets even without a lease;
failed reset retains ownership. Host/compile evidence only for this session layer;
producer stop and native device callback implementation remain caller obligations.

Session ownership now passes shared030 with6 tracked Fast allocations and zero
final owned bytes. Mini map confirms24-bit stereo and16-bit FIFO data ports;
capacity/native bus ordering still need verification before MMIO implementation.

## Editor output-stop binding

A queued editor Studio owner can bind one nonblocking output-stop callback after
start. Edit, Undo, Stop, replacement start and disposal close source ownership,
then send that request once and clear the binding. The binding survives natural
producer completion so a later edit can still abort buffered output. Repeated
Stop is idempotent. No callback is installed automatically in the native app.

The callback can request `amigus_session_stop`; the caller must continue session
steps and confirm detach before freeing output/queue contexts. A stop request is
not reset confirmation or immediate silence. On pump/output failure the caller
must stop both owners; do not close a queue simply because producer pins ended.

Editor output-stop binding passes shared030 with57 tracked Fast allocations,
zero final owned bytes and budget refusal. Physical device stop remains untested.

The injected register-port layer requires exclusive ownership and explicit
verified FIFO capacity. Uncertain writes invalidate alignment until bounded reset
and readback checks succeed. Host/compile checks only; no native MMIO binding.

Register/session integration passes host and shared030 fake-bus tests, including
partial writes and ownership loss. Three tracked Fast allocations, zero final
owned bytes. Mini readback/bus behavior and live playback remain unqualified.

The normal PCM reservation lifecycle now retains the library/card while an
explicit downstream access lease exists. Acquisition failures unwind only their
own references; successful close releases PCM before the library. The native
public-library adapter compiles/links but remains unwired to playback. Host
fake-library checks pass; emulator and real-library behavior remain unqualified.

Reservation/session integration now passes eight host sanitizer checks and
shared030 execution: failed initial reset, Stop with a held buffer, and natural
odd-frame completion retain library/card ownership through pending/failed reset.
The caller detaches before ending its access lease and releasing the card. Every
fake port callback asserts a live reservation. Three tracked Fast allocations,
zero owned bytes; no real library callbacks/MMIO. The production native adapter
is linked but not invoked. Build with `tools/build_amigus_reservation.py`; run
the coordinated shared harness with `--studio-memory reserved-session`. Evidence:
`evidence/enhanced-editor/exec-amigus-reserved-session/`. Native playback remains
unwired; native absent-library runtime and verified device capabilities remain
separate unfinished work.

The discovery-only native adapter probe now handles unavailable amigus.library
on shared030 (two attempts, library=0, closed=1, rc0). Core discovery bounds
enumeration to16 cards, detects cycles and reports known PCM descriptors without
reserving or exposing card pointers. It closes every successfully opened library
reference. Host absent/empty/unsupported/cycle checks pass; positive library/card
and playback behavior remain untested. A discovered PCM descriptor is not a
verified Studio capability. Evidence: `evidence/enhanced-editor/amigus-discovery/`.

## Playback preflight messages (2026-09-24)

Native Play/Pattern now uses the portable, master-preserving Paula snapshot
preflight. Unavailable Studio, AmiGUS and MIDI playback is named explicitly.
High-resolution/stereo/rate, slices and unsupported loop restrictions have
specific messages pointing to preview/render where appropriate; MIDI audio is
explicitly excluded from rendering. This does not enable any new backend or
silently convert a master. Classic eligibility and private mute/solo/name/group
normalization remain unchanged; strict disk export still checks saved metadata.
The refusal happens before replacing existing Paula playback. Live synchronization
retains the existing stop-on-incompatible-edit behavior. No screen layout change.

The native Paula regression now exercises refused new song/pattern requests for
Studio, AmiGUS, MIDI, mixed routes and a separate genuine24-bit master while a
classic song is already playing. It requires continued ticks/DMA, unchanged
active cache addresses/version and retained audio lock; the enhanced master's
PCM and sample descriptors must remain byte-identical. The shared runner now
requires the new PLAY REFUSAL marker as well as normal audio shutdown. Results
and acceptance tier are recorded in the corresponding evidence directory.

Active-replay refusal now passes the shared030 regression (rc0, unchanged
cache/master, continuing ticks; all DMA off at exit). Evidence is in
`evidence/enhanced-editor/paula-play-refusal/`; the original harness return-code
race is retained separately from the successful completion-marker rerun. CIA
fallback execution remains NOT RUN because Workbench owns CIAA timer B. No
physical/audio-listening acceptance is implied.

## Bounded native sample WAV export

The native sample WAV export now streams directly from the authoritative PCM
master through `pt_sample_wav_save`. It retains the declared8/16/24-bit precision
and mono/stereo layout, including unsigned8 WAV coding and RIFF odd-byte padding.
It no longer allocates a whole encoded sample. One fixed workspace contains
paths and two4KiB blocks, allocated through the existing Fast-preferred master
budget. The small header/descriptor stays on the caller stack.

`pt_file_save_generated` writes bounded generated blocks to owned staging, closes
the writer, regenerates and compares every byte, checks EOF, then uses existing
no-replace publication. Generator/write/verification failures clean only owned
staging; allocation refusal creates no file. Source data must remain immutable
for the synchronous call; native export does not process edit events during it.
This does not stream enhanced-project, RAW or IFF exports yet.

Host sanitizer checks compare complete outputs with the existing encoder for
all six precision/channel combinations, verify master preservation and fixed
workspace, and cover allocation/generation/verification failures and existing
destination refusal. Native runtime qualification remains separate.

Streamed master WAV export now passes shared030 using the production Exec pool:
11240-byte fixed workspace,18 Fast/not-Chip allocations, zero owned bytes, budget
refusal. All six precision/channel combinations and failure cleanup pass; no
staging remains. Evidence: `evidence/enhanced-editor/exec-sample-wav-stream/`.
This is native file/allocator evidence, not physical storage or UI acceptance.

## Bounded native RAW sample export

RAW sample export now uses `pt_sample_raw_save` and the same fixed-budget
generated-file transaction as WAV. It encodes directly from the immutable
master, with explicit big/little endian and signed8/unsigned8 options. The
settings must match source bits/channels/rate; mismatches fail before staging.
No resampling or precision conversion occurs. Native RAW export no longer
allocates a whole encoded payload. IFF and enhanced-project saves remain
separate work.

Host sanitizer checks match the established RAW encoder across16 format
combinations and block boundaries, preserve all PCM values, confirm constant
workspace, allocation refusal and existing destination protection. The shared
generated-file layer supplies the previously tested failure/readback cleanup.
Native compile evidence and emulator runtime evidence are tracked separately.

RAW streaming now passes shared030 across16 formats: fixed11240-byte workspace,
35 tracked Fast/not-Chip allocations, zero owned bytes, budget refusal and clean
staging. Evidence: `evidence/enhanced-editor/exec-sample-raw-stream/`. Native editor
handler is connected; no physical storage or visual UI acceptance is implied.

## Bounded native IFF/8SVX sample export

IFF sample export now streams its immutable mono8 master through fixed-budget
generated-file saving. The core8SVX encoder's88-byte header, full32-byte name,
rate, volume, cycles, forward-loop coordinates and odd BODY padding are preserved
exactly. Native sampler eligibility still refuses slices, finetune and unsupported
loop/precision/channel/rate settings; export never converts the master.

WAV, RAW and IFF sample export handlers now avoid whole encoded payload copies.
Enhanced-project saving also uses bounded streaming, as described below.
The synchronous source-lifetime and no-replace/readback/cleanup contract is the
same for all three formats. Host encoder-equivalence and failure tests pass;
native runtime evidence is recorded per milestone.

IFF streaming passes shared030 with11240-byte fixed workspace,11 tracked
Fast/not-Chip allocations, zero owned bytes and clean staging. Exact metadata
and BODY comparisons pass. Evidence: `evidence/enhanced-editor/exec-sample-svx-stream/`.
No physical storage or visual UI acceptance is implied.

## Bounded enhanced-project saving

Native enhanced-project Save now uses `pt_project_file_save`. The allocation-free
core serializer emits the unchanged version1 format in blocks of at most1024
bytes, computing its CRC in a first pass. Orders/events, channel state, MIDI
settings, sample masters at their declared precision, loops/slices and optional
extensions are retained byte-for-byte against the original encoder.

The platform owns one fixed-budget file workspace, runs serialization into
staging, closes it, then serializes again against complete readback before
no-replace publication. Short/overflow/failed producers and mismatches fail and
clean staging. Source lifetime remains synchronous and immutable; no edits are
processed during save. Dirty/saved and recents state changes still occur only
after successful publication. No whole encoded project allocation is required.

CRC and verification add sequential passes over the source; target performance
remains a separate measurement. Core stack workspace is about1KiB in addition
to fixed platform heap workspace. Host exact-format and failure checks pass;
native runtime proof is recorded separately.

## Bounded classic MOD export

Native MOD export now uses `pt_mod_file_save` with the same bounded staging and
complete readback verification as enhanced-project Save. `pt_mod_export_stream`
keeps direct, explicit rounded8 and fixed-seed TPDF8 conversion policies distinct;
all pre-existing classic eligibility constraints still apply before any output.
The encoded copy is derived directly from immutable masters, never a playback
cache. Export does not mark the richer project saved or consume undo history.

The core uses a1084-byte header and1024-byte block; the platform workspace is
fixed7184 bytes on the host. Legacy headers, pattern ordering, loop metadata and
cross-sample dither sequence match the original encoder. Multiple native-source
passes occur for validation and verification; physical performance is unmeasured.
Host sanitizer tests cover65 patterns, multi-block8/16/24 masters, all three
policies, legacy/no-legacy headers, interrupted/short readback, sink failure,
allocation refusal, destination protection and unchanged masters. Native build
and runtime qualification are reported separately. Emulator availability is
currently unverified after an identity-guard refusal; no bypass or restart.

## Converter utility memory

`PT24GConvert` now uses the bounded platform input reader and releases the
encoded input immediately after transactional document decode. Project and all
MOD export policies reuse the verified streaming save paths; no whole encoded
output allocation is retained. The input-size limit remains64 MiB. Native builds
use the production master allocator, querying current Fast RAM and sharing the
ceiling across input, decoded master storage and save workspace. Fast exhaustion
does not spill into Chip; existing Chip-only fallback/reserves still apply.

Host sanitizer tests cover exact MOD/project roundtrip, unchanged destinations,
invalid input, explicit rounded/TPDF policies and a700000-byte allocation limit
with a maximum-length classic sample. All owned memory is released. Native
cross-build passes; native runtime and physical performance are not yet proven.
Import still requires encoded input alongside decoded staging while validating;
this change reduces lifetime and export peak, not streaming decode.

## Bounded RAW sample import

Native explicit RAW import now calls `pt_raw_file_import`, measuring the file
against the existing64 MiB limit and reading3072-byte stack blocks directly into
an unpublished sampler master version. It does not allocate the encoded file.
All16 existing8/16/24-bit mono/stereo/endian/signedness combinations decode through
`pt_raw_decode`; frame alignment and explicit format rules remain unchanged.
EOF and successful close are required before publishing the version. A failed
read, decode, allocation or journal commit releases staging and preserves the
previous sample/history. Normal undo/redo and generation invalidation apply.

The synchronous `pt_sampler_import_raw_fill` seam permits a checked producer to
fill the staged master; it must initialize every value, finish its input checks
and not modify project/history/source while borrowed. Existing in-memory RAW
import reuses this transaction. Host sanitizer tests cover multiblock exact
values for16 formats, three allocation failures, fill/size refusal, interrupted/
short reads, undo/redo and zero leaks. WAV/IFF/project/PP20 imports still retain
encoded input; this milestone does not claim general streaming import.

Project and MOD streaming save fixtures now pass shared030 after fresh guards:
fixed7164-byte native file workspace;6 and12 Fast/not-Chip allocations respectively,
zero owned bytes, no staging left, exact cleanup and explicit release. Evidence
`exec-project-stream/` and `exec-mod-stream/`; earlier identity refusal preserved.
This qualifies these file/allocator fixtures, not visual editor or physical I/O.

## Bounded WAV sample import

Native WAV import now sniffs only the12-byte RIFF/WAVE identity, then uses
`pt_wav_file_import`. The same core WAV inspector serves memory and positional
readers; metadata reads are at most16 bytes, with the existing chunk/rate/format/
frame-alignment/duplicate/padding acceptance rules. Unknown chunks are skipped.
Six8/16/24-bit mono/stereo formats decode via3072-byte blocks directly into the
unpublished sampler master. WAV8 is unsigned;16/24 remain exact signed values.
The full encoded file is never allocated. File length/EOF and close are checked
before commit. Undo and failed-import preservation use the existing transaction.
WAV embedded loop metadata remains ignored as before; this adds no new formats.

RAW uses the same reader, now rechecking complete file length before close.
Both paths require a stable input file for the synchronous operation; concurrent
same-length content edits are not promised snapshot semantics. Host sanitized
format/undo/refusal tests and reader edge cases pass; full native build passes.
IFF and module/PP20 inputs remain whole-file and are separate remaining work.

RAW/WAV import fixtures pass shared030 with160/60 production Fast allocations,
zero owned bytes, budget refusal without Chip fallback, exact cleanup and
explicit release. Evidence: `evidence/enhanced-editor/wav-import-stream/`.
Native handler wiring is compiled; visual UI/physical I/O remain separate.

## Bounded IFF/8SVX sample import

Native IFF import now sniffs FORM/8SVX then reads through the shared core
positional inspector and decoder. Metadata workspace is32 bytes; audio is read
in256-byte blocks, including stateful Fibonacci-delta decoding across blocks.
Both supported compression modes use unpublished sampler master storage; no
whole encoded input allocation is needed. Existing single-octave/mono/CHAN,
VHDR/BODY/NAME ordering, padding, loop/trailing-PCM and format constraints remain.

`pt_sampler_import_svx_fill` preserves embedded NAME (filename fallback), rounded
tracker volume and forward-loop endpoints before one transactional commit.
Cycles remain outside sampler metadata as before. Allocation, read/decode and
journal failures preserve the old master/history. File-length/EOF/close checks
precede commit; input must remain stable during the synchronous operation.
Host sanitizer tests compare compressed stream results with the original decoder,
cover metadata/no-metadata and loop/no-loop combinations, multiblock samples,
three allocation failures, fill/limit/multioctave refusal, interrupted/short reads,
undo/redo and zero leaks. Native/runtime evidence is recorded per milestone.
Module/project/PP20 imports still use encoded input buffers.

IFF import passes shared030 with80 production Fast allocations, zero owned bytes,
budget refusal without Chip fallback, exact cleanup and explicit release.
Evidence: `evidence/enhanced-editor/svx-import-stream/`. This qualifies file,
metadata, undo and allocator behavior; visual UI/physical I/O remain separate.

## Bounded uncompressed MOD document load

Native document Open/load now recognises classic M.K./M!K! files while excluding
enhanced-project and PP20/PX20 signatures from this route. `pt_mod_file_load`
uses shared strict MOD preflight through positional reads (header1084 bytes,
pattern/sample blocks1024). The whole encoded MOD is no longer allocated.
Decoded orders/events/sample masters and preserved classic header are allocated
into a separate candidate with the existing document allocator and budget.
Final length/EOF/close verification must succeed before replacing the old
project. Reader, allocator, decode and finish failures release only staging.

The core reader decoder is explicitly staging-only: a failing read may write a
prefix of candidate storage. Disjoint destination capacities and metadata bounds
are checked; callbacks must be non-reentrant and source contents stable. Strict
warnings/format limits, all128 order entries, signed PCM and classic metadata
remain unchanged. Existing in-memory import keeps its no-partial-output contract.
The shared staging allocator serves both paths, including PP20 budget accounting.
Enhanced-project/PP20 and module donor-preview input still use encoded buffers;
this milestone changes native full-document uncompressed MOD loading only.

The command-line converter also selects this bounded uncompressed MOD loader.
Its maximum-length classic-sample fixture now converts to MOD and enhanced
project under a 550000-byte allocator ceiling (host peak 538110, final zero),
which excludes keeping a complete encoded MOD alongside decoded masters.
Enhanced/PP20 converter input still follows the existing whole-input path.

MOD source/donor preview also selects bounded file loading for uncompressed
MODs. The editor's synchronous source-loader transaction loads into a separate
unpublished document within the remaining sampler budget. Only a completed load
replaces the previous donor; failure frees staging, keeping the old source,
destination project, selection and undo intact. Samples imported from the donor
remain independent master copies with existing undo ownership. PP20 donor input
still uses the previous unpacking path. Donor UI/runtime acceptance is separate
from the host source/import regression and native cross-build.

### Enhanced-project positional reader core (integration pending)

`pt_project_probe_reader` validates the existing version-1 format through bounded
positional reads (maximum 1092 bytes; CRC blocks 1024). It retains strict CRC,
chunk/version/flag/padding, capability, sample, slice and event checks.
`pt_project_decode_reader` writes master PCM, slices and optional extensions to
unpublished caller-owned staging, with capacity and disjoint-storage checks.
Exact 24-bit/stereo masters are retained; an I/O failure may change staging but
never publishes the output descriptor. Source bytes must remain stable and
separate from destination storage throughout the synchronous operation.

Host sanitizer tests compare preflight results with the original contiguous
parser for truncations and checksum-repaired mutations; every injected read
failure preserves output descriptors. The mixed project re-encodes byte-for-byte.
These reader APIs are not yet connected to document/file loading: native editor
and converter enhanced-project imports still allocate a whole encoded input.

### Enhanced-project transactional file loading

Enhanced-project full-document loading in the native editor and converter now
uses the positional reader. `pt_document_load_project_reader` probes, allocates
separate master staging within the supplied budget, decodes, then requires a
successful source finish callback before replacing the old document. Any failure
releases staging only. The platform adapter checks file length, EOF and close
before success; a fixed 4096-byte read cache avoids per-field disk seeks. No full
encoded enhanced-project buffer is allocated by these paths. Source stability
remains required; this is not a concurrent-write snapshot protocol.

Host mixed-project import tests verify exact master re-encoding, every read and
allocation failure, finish failure and size/budget refusal preserving the old
document. Short/interrupted reads are covered. Converter tests now load the
large enhanced project and export MOD under the same 550000-byte allocator
ceiling, peak538110 and final zero. PP20 still retains packed/unpacked buffers.
These statements supersede the earlier integration-pending notes above.

### PP20 reverse file reader

Native document load, MOD donor preview and the converter now read PP20 packed
input through a 256-byte reverse codec cache and a fixed 4096-byte file cache.
No full packed-input allocation remains in those paths. Back-references still
require a complete unpublished unpacked buffer: its allocation and decoded master
staging both count against the supplied budget. Only strict unpacked MOD content
is accepted; encrypted PX20 and nested/enhanced payloads remain unsupported.
Length/EOF/close must succeed before document replacement. Failed read, finish,
allocation or budget checks release scratch/staging and preserve the old project.
The old contiguous API remains for callers that already hold immutable bytes.

Host comparisons cover all skip counts0..32, short literal lengths, back-reference
length/offset forms and mutated malformed streams. Every injected reader failure,
all seven allocation failures and finish failure retain the old document; short
and interrupted file reads also pass. Large packed-MOD converter input now peaks
at664104 allocator bytes under700000, final zero, excluding the full packed input
beside scratch and masters. These results supersede earlier PP20-buffer notes.

### Standalone renderer allocator integration

PT24GRender now uses the production queried Fast-first allocator on Amiga for
masters plus bounded WAV/stem workspaces. Host builds retain malloc-backed
allocation. MOD/enhanced/PP20 input selects the positional loaders instead of
fread/whole-input malloc. The64MiB document policy remains, alongside the native
allocator's actual free-memory/reserve policy. Unsupported inputs are rejected
without allocating their payload. No-replace publication and rendering semantics
are unchanged; offline rendering is not live Studio transport acceptance.

Host allocator-ceiling tests show MOD/enhanced peak542838, stems546414, PP20
peak664104, all below700000 with zero owned bytes afterward. Every allocation
failure across mix/stem paths refuses publication, removes owned staging and
preserves existing destinations. MOD/enhanced/PP20 WAVs match exactly; retained
native finetune WAV references also remain byte-identical.

Native stem CLI is now additionally exercised in shared030: two aligned24-bit
stems match host bytes, and intentional re-export to the existing directory
returns refusal with files unchanged. The owned staging directory is checked
absent on host failure paths and after native completion. This is software
runtime evidence, separate from editor UI and physical performance acceptance.

### Paula Chip-RAM reserve admission

Native Paula sample-cache allocations and the silent DMA word now leave a
512 KiB Chip-RAM display/system reserve, matching the Chip-only master policy.
The initial cache ceiling excludes that reserve; every new allocation rechecks
current free Chip RAM, including memory consumed by other applications since
startup. Allocator refusal lets the cache evict unpinned entries and retry;
active leased buffers are never evicted. If the silent word cannot be allocated
on restart, idle retained caches are released before one retry. Exhaustion uses
the existing clear out-of-Chip-memory error and cleanup path, preserving masters.

The reserve is an admission policy, not an OS reservation against concurrent
allocations. Fragmentation can still make AllocMem fail. Host sanitizer tests
exercise exact reserve boundaries, changing free memory, allocation failure,
eviction, pinned data preservation and complete release. Emulator and physical
acceptance of the changed native bridge are tracked separately in evidence.

### Bounded Paula live-sync comparison

Paula now retains one immutable full classic export plus two header/pattern
prefixes (active replay and unpublished row staging). The former second full
export is removed: `pt_paula_sync_prepare` streams direct MOD bytes in bounded
blocks, comparing sample headers and all sample payload bytes against the
immutable export. It stores only the prefix and checks the instrument working
set before any row is published. A mismatch stops replay/requires rebuild, as
before. EFx mutations remain isolated in private Chip playback copies.

This saves exactly the encoded sample payload size from the replay allocator,
including unused project samples; it does not remove the remaining immutable
snapshot. Sync still examines the master PCM synchronously, so no reduced CPU
cost or physical performance claim is implied. Input must remain stable during
this synchronous operation. Failed preparation may change only unpublished row
staging; the native caller publishes rows solely after complete success.

## Donor workflow and production allocator qualification

The clean native editor now passes MOD/PP20 donor preview, cancel, selected
instrument import, malformed-source refusal preserving redo, exact metadata/PCM
save, whole-song MOD export and project reopen. Separately, PTExecSourceTest
reuses the source ownership/failure matrix through the production master pool:65
Fast/not-Chip allocations and zero owned bytes at disposal on shared030.
Evidence is in donor-shared-qualified and donor-exec-memory under
evidence/enhanced-editor. These do not qualify unrelated display edits or
physical memory/performance. Runtime library internal allocations remain outside
the explicit allocator accounting.

## Unsupported sample input does not allocate a whole-file buffer

The native editor now calls pt_editor_sample_file_import in platform/sample_import.c.
RAW/WAV/IFF/MOD/PP20 retain bounded reader paths. Unknown formats are refused
before whole-file allocation or sample/history mutation; donor-only selection
does not import WAV. Host sanitizer and shared030 production-allocator tests
verify a1MiB invalid input causes no explicit allocation, exact stereo24-bit
import and retained redo after refusal, with zero owned bytes at disposal.
See evidence/enhanced-editor/sample-dispatch. Native build passes; unrelated
display edits and physical behavior are not qualified by this non-UI fixture.

## Native stereo24 sampler roundtrip

Clean1d0e0ab shared030 UI qualification now verifies true24-bit stereo import,
reverse/undo/redo, malformed replacement preserving redo, byte-identical WAV
export, exact saved master PCM and CRC, project reopen, identical re-export
and project save. Low sample bits remain intact throughout. Evidence is in
evidence/enhanced-editor/stereo24-shared-qualified. This is native editor/file
workflow acceptance, not live Studio/AmiGUS output or physical performance.

## Unknown project inputs

Native editor full-document loading and the converter now refuse unknown
formats after bounded signature probes, rather than allocating an entire input
just to reject it. Recognized MOD, packed MOD and enhanced projects retain the
transactional bounded readers. Converter sanitizer tests verify an unsupported
sparse file larger than64MiB has zero explicit allocation and produces no output;
supported roundtrips and explicit16/24-to8-bit export remain valid. Full native
build passes. No fresh native runtime/UI qualification is claimed for this small
refusal change. The host converter target and round8 test now link the reader
modules they use.

## EFx offline exports and generated samples

Offline selected-track and grouped/individual stem renders retain every
channel's shared-sample mutation clock, regardless of audio selection/mute/solo.
EFx mutates a budgeted private sample bank, not source masters or playback caches.
Each planning, output and verification pass starts afresh. The sampler's EFx
bounce appends its generated stereo16/24 master only after the existing atomic
sample/history transaction succeeds; source masters and redo survive failure.
The opt-in API passes shared030 production allocator checks:137 Fast/not-Chip
allocations and zero owned bytes after completion, including failure paths.

The native editor selects this path when the requested song/pattern contains
EFx. Its additional private-sample budget comes from the current available
master-memory pool; each allocation rechecks live available memory, the shared
ceiling and reserve. There is no assumption that128MiB is free. This path retains
the bounded whole mono8 one-shot or forward-loop/no-slice/no-interpolation subset;
its generated output may be stereo16/24. Other high-resolution projects keep
the ordinary renderer. Queued Studio EFx and physical/audio acceptance remain
separate. Native UI evidence is tracked with the exact candidate.


### One-shot EFx playback copies

The offline EFx path now accepts mixed whole mono8 forward loops and one-shots.
Following the pinned replay's `mt_Init`, it clears the first two samples of each
one-shot **only in its private bank**. It plays the full initial sample, then
retains the two-frame repeat even after EFx turns its zero bytes into audible
values. Fresh notes, E9x retriggers and EDx delayed notes use that same private
representation. Each render pass reconstructs it from the unchanged master.
This is an explicit classic playback conversion; enhanced save, undo and ordinary
sample processing still use the original master bytes, including a nonzero first
word. The queued Studio path retains its EFx refusal. Existing restrictions on
cross-instrument handoffs also remain; this does not widen that subset.


The clean-layout native editor built from `91dc9b9` now has mixed one-shot/loop
workflow evidence in `evidence/enhanced-editor/invert-mixed-workflow`. WAV and
individual stems match host bytes; selected-track bounce creates an exact stereo24
master. All 31 original serialized sample records remain identical after project
save, including the one-shot's original nonzero first word. Undo/redo and clean
exit pass. This qualifies those emulator workflows for the recorded binary only;
physical playback/performance and queued Studio EFx remain separate.


## Private EFx queued producer core

`pt_render_invert_session` is an explicit incremental producer for the existing
copy-owned Studio pump/queue. It stages the same bounded whole mono8 one-shot and
forward-loop subset as the offline EFx renderer into its own mutable bank. EFx
runs only at consumed interval boundaries; queue backpressure prevents further
producer advancement. Selection/mute/solo continue to affect audio, not another
channel's shared-sample mutation clock. Editable masters and ordinary immutable
Studio source pins are unchanged.

The producer mixes at most256 stereo24 frames at48k per pull into borrowed scratch
that the pump copies. Stop/end/error discard private voices before releasing their
bank. Already queued PCM is independent; a consumer-held block survives producer
closure, and queue destruction still refuses until the consumer releases it.
Open uses the supplied bounded allocator, with a separate private-sample budget;
pull allocates nothing. Natural end/error may call allocator release. These are
serialized owner-thread operations, not interrupt-safe or real-time qualification.

Project metadata/events/orders must remain immutable and live until stop. The
core does not yet bind sampler generations or stop automatically on editor edits.
The sampler/editor Studio entry points and native PLAY therefore still refuse
EFx. Wiring an explicit opt-in producer through those generation/lifetime guards
is the next integration step.16/24-bit sources are not silently converted to
satisfy this classic EFx subset; ordinary Studio mixing retains its direct master
precision. Actual AmiGUS transport and physical acceptance remain separate.

Host regression compares every queued PCM frame against offline rendering for
normal playback, lead-in and row-range pre-roll using1/17/256-frame partitions,
with submit refusals, delayed completion, held-lease cancellation failure, repeat
sessions, every open allocation failure and sample-budget refusal. Masters remain
byte-identical, including nonzero one-shot first words.

The native core fixture also passes on shared030 with the production allocator:
53 Fast/not-Chip allocations, zero final owned bytes and budget refusal without
Chip fallback. It exercises256-frame playback plus17-frame held-lease stopping.
Evidence: `evidence/enhanced-editor/invert-queued-core/`. The coordinated window
completed RC0, all4DMAoff, exact staging cleanup and explicit release.


### Sampler binding for private EFx

`pt_sampler_invert_song` explicitly wraps the private producer with the sampler's
version and project-table identity guards. Opening copies classic playback data
without promoting, rewriting or reducing a master. Pull refuses a changed sampler
generation or replaced samples/events/orders/sample count before touching the
producer again, releases its private voices/bank, and keeps the error sticky.
Stop and normal end release the producer; an already copied queue block remains
owned by its queue. Allocation failure preserves the caller's output pointer and
all sampler history/master state.

The owner must still stop before editing, undo, document replacement or disposing
sampler/project objects. This defensive binding does not intercept edits or make
in-place pattern mutation safe. It is an opt-in API; existing editor Studio/native
PLAY paths are unchanged. Connecting it to the editor's before-change and queued
output-stop guards remains the next step. Host checks cover stop/edit/restart,
undo/redo, unexpected version changes, all four table/count guards, every open
allocation failure, budget refusal, unsupported24-bit preservation and completion.

The sampler binding also passes on shared030 with93 Fast/not-Chip allocations,
zero owned bytes and budget refusal. Evidence:
`evidence/enhanced-editor/invert-sampler-song/`. Editor interception and real
output transport remain unimplemented by this adapter.


### Editor ownership of private EFx sessions

`pt_editor_studio_start_invert` and `pt_editor_studio_start_invert_queued` are
explicit budgeted entry points for classic EFx private-bank playback. They share
the editor's existing before-change guard and output-stop callback. Pattern or
sample edits, undo, dispose and explicit Stop close private voices/bank before
masters or metadata change. Held queue blocks remain independent until their
consumer releases them; waiting/pending audio is discarded. Natural completion
instead drains copied output and retains its output-stop binding for later edits.

Producer or queue failures now also invoke the bound output-stop request once;
previously the pump closed only its producer. Invalid block-size requests alone
still leave a valid session unchanged. Output cancellation remains asynchronous:
the output owner must confirm completion/detach before releasing its context or
a held queue block. Ordinary Studio start keeps its immutable-master behavior.

These are editor-owner APIs, not native PLAY wiring, a new screen or an enabled
AmiGUS device. The native application's unavailable-Studio indication remains
accurate. Classic EFx still refuses16/24-bit sources instead of downgrading them;
ordinary Studio retains direct full-precision master mixing. Tests exercise actual
pattern/sample edit and undo keys, stale generation failure, dispose, queue leases,
output stop including an odd FIFO tail, natural drain and budget/format refusal.

Clean-source host integration checks and the native shared030 fixture pass.
The production allocator reports111 Fast/not-Chip allocations and zero final owned
bytes, including edit/undo/dispose and held-output cases. Exact build and RC0/
DMAoff/cleanup evidence: `evidence/enhanced-editor/invert-editor-owner/`.


### EFx instrument handoffs from private playback banks

Explicit EFx offline and incremental sessions now accept instrument-only changes
between compatible classic mono8 samples at the same sample rate, including a
one-shot whose master begins with nonzero bytes. Admission uses the private
playback contract: one-shot initialization clears the playback word only. The
master is neither rewritten nor used as a silence test for this private path.
Ordinary rendering retains its existing first-word and EFx restrictions.

Pinned 2.3F captures establish that changing instruments resets the EFx loop
cursor while retaining the speed and accumulator. A same-row EFx command then
changes the speed; an instrument-only row without EFx inherits its clock. The
current voice iteration continues until its next repeat boundary, where it
adopts the new instrument's loop or two-frame one-shot repeat. Returning to the
old instrument preserves the private mutations already made to it.

Evidence in `evidence/enhanced-editor/invert-handoff/` contains four MOD fixtures,
each captured twice on shared030 with the pinned replayer. Raw logs retain actual
allocation-relative addresses; normalized traces use one fixed mapping per sample
cache. Tests check exact repeat parity without hiding cursor or PCM differences.
A separate PCM oracle consumes the recorded periods, volumes, repeat registers
and complete loop-byte snapshots. All four fixtures match every output frame in
offline rendering and incremental pulls of1,17 and256 frames, with two repeat-source
changes each. Both master arrays remain exact. Rate mismatch, interpolation and
16/24-bit EFx inputs still refuse before output; ordinary true24 Studio remains
separate. This change does not add native PLAY/device output or physical audio
acceptance, and does not relax the existing slice/format/loop limits.

The same four PCM oracle cases also complete successfully on shared030 using the
production Fast-memory allocator:41 Fast/not-Chip allocations and zero owned bytes
per case, including budget refusal. Native binary SHA256:
`83a6f0a3e2cb734aa174caed214214cf1716a1a3ab3c0f64659547febc13375f`.
The original120-second harness deadline was missed; the already running finite
script completed during an explicitly coordinated passive extension. Both the
original timeout and later functional completion are retained under `native/`.
This is functional/native allocation evidence, not a realtime performance pass.
All DMA was off before exact run/launch cleanup and explicit resource release.
Future invocation of this expensive one-frame pull oracle reserves420 seconds.


### Handoff export/bounce/save transaction qualification

The newly admitted EFx handoffs now have end-to-end host coverage through actual
WAV/stem/grouped export, cancelled verification, existing-destination refusal,
bounce cancellation after an instrument change, successful bounce, undo/redo and
project-file save/reload. All four pinned fixtures match independent trace PCM.
Source serialization is unchanged, saving after undo reproduces the entire
pre-bounce file, and a reloaded project preserves both source masters and the
new stereo24 bounce. A refused private-bank budget preserves pending redo.

A clean335f981 native editor candidate passes the corresponding existing controls
on shared030, with exact WAV/stems/bounce and all31 source sample records intact.
The accepted classic layout remains unchanged. Evidence and build identities:
`evidence/enhanced-editor/invert-handoff-workflows/`. Emulator application checks
remain distinct from physical output, timing and AmiGUS acceptance.


### EFx handoff command combinations

Six further pinned-reference fixtures combine an inherited EF8 mutation clock
with E92 retrigger, an ED3 note, or a 901 note while changing to a looped or
one-shot instrument, then returning to the first instrument. Each capture is
repeated. The existing production implementation matches these cases without
widening admission or changing master data.

The ED3 cases retain the old period until the delayed trigger, while the new
loop and mutation clock are already selected. E92 restarts on ticks0,2,4 without
resetting the inherited EF8 accumulator. The offset cases distinguish a256-byte
one-shot start from an offset beyond a loop's initial end (two-byte fallback).
They also retain the reference's second offset application after triggering.

Reference normalization binds each allocation from its stable loop pointer and
MOD loop metadata, so changing9xx start addresses remain visible. Raw captures,
fixture identities and mutation-detection checks are retained in
`evidence/enhanced-editor/invert-handoff-commands/`. Host sanitizer tests compare
all17280 PCM frames, offline and pull1/17/256, and actual WAV/stem/group/bounce/
undo/redo/save transactions. All original masters remain exact; ordinary EFx,
16/24-bit EFx, interpolation and rate-mismatch refusal checks remain in place.
This qualifies these six cases, not every effect combination or physical audio.

All six also pass the native Fast-memory PCM oracle on shared030 within its
420-second reservation:41 Fast allocations per case, no Chip fallback and zero
owned bytes at completion. `native/build.json` and raw RC/log records pin the
exact diagnostic. All DMA was off before guarded cleanup and resource release.
This is a core/software check, not a new native-editor UI or physical-output pass.


### Shared EFx clocks during instrument changes

A separate188-byte reference diagnostic retains the earlier164-byte schema and
appends channel1's cursor, speed, accumulator and16 loop bytes (or the two-byte
one-shot repeat). This observes both private banks when an unheard channel keeps
mutating the former sample after the heard channel selects another instrument.
The shipping replay and original diagnostic output are unchanged.

Three fixtures cover clocks joining on one sample, separating onto different
samples, and a one-shot destination followed by a return. Repeated pinned2.3F
captures agree after fixed per-allocation relocation. Both channels' clocks and
both loop snapshots match the portable sequence; snapshots of an aliased sample
agree. The independent PCM oracle applies every recorded bank before each tick.
Every17280-frame output matches offline and pull1/17/256. Muting the contributing
channel or soloing the heard channel leaves the expected audio unchanged; the
base test also excludes the contributing channel from the selected track mask.
Actual WAV/stems/grouped stems, bounce/cancellation, undo/redo and save/reload
checks preserve every master. This qualifies existing behavior, without adding
new production effect admission or native Studio output.

Evidence and exact diagnostic identities are in
`evidence/enhanced-editor/invert-shared-handoff/`. These bounded classic mono8
fixtures do not claim arbitrary loop-size coverage, analog output or realtime
physical acceptance;16/24-bit masters remain outside private EFx mutation.

The native shared030 PCM fixture also passes all three cases, including muted
and solo-excluded contributors:49 Fast allocations per case and zero final owned
bytes, no Chip fallback. Exact build/RC/log/cleanup evidence is in `native/`.
The run finished within420 seconds and was explicitly released after all DMA
was off and owned run/launch absence confirmed. No physical timing or audio claim.


### Actual EFx writes beyond the short-loop snapshots

A separate208-byte pinned diagnostic now records ordered address/value/channel
entries at the actual reference EFx byte store. Its fixed eight-entry ISR buffer
fails on overflow and is cleared at each tick. The previous16-byte shared-bank
baseline agrees with its earlier replay state and loop snapshots. Three further
fixtures cover18/64/256-byte loops, wrap, same-tick shared-address collisions and
instrument changes. The host compares complete private banks each active tick
and every stereo24 output frame offline and in1/17/256-frame pulls, including
muted/solo-excluded mutators. All original master samples remain unchanged.

This removes the oracle's dependence on a16-byte snapshot for these cases;
it does not impose or remove a product loop-size cap. Evidence and diagnostic
identity: `evidence/enhanced-editor/invert-write-events/`. Production behavior
and private EFx16/24-bit refusal are unchanged; no live device output is enabled.

Actual WAV, individual/grouped stem and bounce workflows also match the recorded
writes for all four fixtures. Cancellation, undo/redo and enhanced-project
save/reload preserve the master data; generated bounce output remains stereo24.

Native shared030 checks also completed all four cases with44 Fast allocations
per case, zero owned bytes and no Chip fallback. The batch missed its420-second
bound and completed naturally at451.8 seconds during separately coordinated
read-only observation. Both results are retained; it is not an in-bound pass.
All DMA was off before exact cleanup and explicit release. Native pull partitions
were17/256; host includes1. No physical performance or audio claim follows.


### Four mutation clocks with pattern delay

Two further pinned-reference fixtures combine all four private EFx clocks with
EE2/EE1 delayed rows, shared-bank collisions, instrument changes, cursor wrap
and EF0 disable/re-enable. Repeated tick zero produces seven ordered stores
(or five with one clock disabled), within the diagnostic's fixed eight-entry
buffer. Full-bank and independent PCM comparisons agree, including all three
unheard clocks when excluded, muted or solo-excluded. WAV/stems/groups, bounce,
cancellation, undo/redo and project save/reload preserve every original master.

Evidence: `evidence/enhanced-editor/invert-four-clock-delay/`. These are bounded
classic mono8 cases; no production source changes or new effect admission were
needed. Native Studio output and physical acceptance remain separate.

Both cases also pass native shared030 offline/pull17/256 and mute/solo checks:
44 Fast allocations each, zero final owned bytes, no Chip fallback. Each ran in
its own180-second window with no timeout; DMA-off, exact cleanup and explicit
release passed. The full23-test host sanitizer suite includes one-frame pulls.
No actual device output or physical timing is qualified by these results.


### Reference audio through editor queue ownership

The four-clock EE2/EE1 fixtures now have independent PCM checks through the
editor Studio owner, sampler binding, private producer, pump and queue. Full
queue backpressure preserves pending data; a consumer-held block stays valid
through pattern/sample editing. The output-stop callback checks that producer
ownership is already closed and the original serialized project is still intact
before the edit applies. Undo restores the exact project and a fresh complete
playback matches the reference again. Natural end drains all copied PCM; queue
close refuses until a held lease is released. Budget refusal and enhanced
serialization roundtrip preserve masters and no tracked allocations remain.

The fixture uses committed production source exports to exclude unrelated local
display work. See `evidence/enhanced-editor/invert-editor-reference/` for the
build and test boundaries. It exercises the editor/controller ownership API;
native PLAY, real consumer cancellation and physical output remain separate.

All 26 related host sanitizer tests pass. Both editor reference cases also pass
native shared030 in separate 240-second windows (93.62 and 78.76 seconds), with
77 Fast allocations per case, zero final owned bytes and no Chip fallback.
Completion, all four DMA channels off, exact cleanup and explicit release were
verified. No timeout, retry or reset was needed; no device output was enabled.

### Bounded wavetable address allocation and upload

`amigus_sample_ram` now provides the cache's address allocator and bounded upload
callbacks with an injected register bus. It takes an explicitly verified region,
uses fixed metadata and aligned allocations, preserves pinned addresses under
fragmentation/eviction, and uploads converted8/16-bit bytes in bounded chunks.
Partial final words are padded within the derived allocation. Failed writes
never publish an incomplete resource; the original master remains unchanged.
See `AMIGUS_SAMPLE_RAM.md` for the lifetime contract and pinned register facts.
Native MMIO, voice dispatch and physical tests remain absent. Explicit wavetable
reservation/cache lifetime is now implemented as described below; default PCM
reservation must not be mistaken for wavetable ownership.

### Wavetable reservation and cache lifetime

Explicit resource selection now passes through core/native reservation callbacks;
default open and discovery remain PCM-only. `amigus_wavetable_cache` requires a
wavetable reservation, keeps its access lease through cache retirement, and
refuses detach until all voice-held sample leases are released. Wrong-resource
attach, stale ownership (including cache hits), partial upload failure and pinned
old generations are checked with a fake library and injected bus. Masters retain
24-bit data. No real library/card access, native voice dispatch or stop confirmation
is established by this software integration. See `AMIGUS_SAMPLE_RAM.md`.

### Sampler-driven wavetable invalidation

The sampler now has a dedicated wavetable bridge that pins the current immutable
master during upload and retires cached copies before a new trigger after any
sampler generation, slot count or table identity change. Edits and undo/redo
rebuild derived representations; old active device leases preserve their bytes
until explicit unpin. The bridge refuses old-generation address lookup, nonempty
cache adoption and revision wrap. Busy close keeps reservation ownership alive.
Tests preserve exact true24 enhanced-project bytes after promotion and undo,
including import, metadata changes, slot growth, upload failure and document
close/rebind. This is software integration with a fake bus; native device/voice
output remains unwired. See `AMIGUS_SAMPLE_RAM.md` and
`evidence/enhanced-editor/sampler-wavetable/`.

### Wavetable whole-sequence capability analysis

The mono wavetable dispatcher now has a bounded silent whole-sequence pass using
its same capability rules. It catches late stereo, segment and repeat operations
before a caller starts output, including operations in pre-roll. Both plan and
sequence workspaces use the caller's allocator and are released on refusal,
resource failure or completion. Master PCM is preserved; no cache upload or
voice/device callback occurs. Detailed refusal reports retain renderer errors.
This is a callable analysis layer, not yet a mandatory native playback gate:
the owned session and editor barrier described below enforce this gate;
row-range pre-roll and actual native device integration remain unfinished. See `AMIGUS_SAMPLE_RAM.md` for the contract and evidence.

### Owned wavetable song sessions

`wavetable_song` now enforces successful complete capability analysis before it
pins triggered masters or starts output. It pins only actually triggered slots,
copies options/format, checks revisions/settings at each interval operation and
retains master/controller/device resources through pending stops. Failed open,
normal end, stale generation and uncertain starts have bounded cleanup paths.
Masters retain their original8/16/24-bit precision and enhanced-save bytes.
Project arrays remain borrowed/immutable by contract. The editor barrier below
protects mutations; row-range pre-roll is now integrated as described below.
Native output integration remains unfinished. No real AmiGUS access or output is enabled by this owner.

### Confirmed-stop editor mutations

The wavetable song can now own the editor change barrier. Pending or failed
stops block pattern/sample/song changes, import, undo/redo and disposal, retaining
all referenced storage until confirmation. Native replacement/quit/bounce paths
honor the same result; the existing synchronous Studio hook remains supported.
Cursor/channel navigation stays available. This is software controller integration
with injected device callbacks; native wavetable PLAY/MMIO/output remains disabled.
Callers must detach the owner before editor memory is freed or reinitialized.

### Exact interval-start voice snapshots

`pt_render_sequence_snapshot` now exposes a bounded copy of the software voices
and resolved per-side gains after `next`, before any interval frames are consumed.
It preserves fractional Q32 position, loop state and borrowed source descriptors;
no source read, allocation, pin or master edit occurs. Mid-interval, completed,
failed and private/mutating sequences refuse without changing the output object.
The source/session owner must retain all borrowed descriptors and PCM storage.

This establishes state reconstruction for row-range starts. Host/native fixtures
compare resumed 24-bit PCM exactly against continuous Studio and offline reference
rendering, including silent pre-roll, forward/ping-pong loops and tempo/delay.
It does not make the existing AmiGUS integer-start command capable of restoring
fractional phase. Wavetable row-range playback requires the explicit restore
path described below; no rounding or audible pre-roll fallback is allowed.

Exact wavetable restore preparation now retains fractional sample position in a
separate Q32 cache-byte cursor, preserving8/16-bit cache conversion without
changing24-bit masters. A bounded whole-snapshot check requires explicit restore
capability and validates every active source/geometry before future upload/start.
The preparation functions have no cache/source ownership or device effects.
Session integration follows below; native phase semantics remain unverified.
See `AMIGUS_SAMPLE_RAM.md` and `evidence/enhanced-editor/amigus-restore/`.

The injected wavetable owner now supports exact restore callbacks with all active
playback copies acquired before any voice starts. Failed preparation releases
its temporary leases; uncertain restore retains each started cache until confirmed
stop. Idle-owner and current-version checks prevent overwriting active ownership.
Masters keep their original precision and enhanced-save bytes. This is a software
lifetime path; native driver wiring remains open.

### Row-range wavetable sessions

Sessions with an explicit exact-restore callback now consume silent pre-roll in
bounded blocks without voice commands or cache uploads. Before acquiring master
pins or publishing output, session preflight validates the entire command sequence
and the first emitting snapshot. Only active restored samples and future emitted
triggers receive source pins; a sample finished during pre-roll stays out of the
playback cache. Project descriptors remain borrowed and immutable throughout.

At the first emitting interval, `next` restores the exact snapshot before returning
its frame count, even when that interval is also the final interval. The caller
advances silent spans without waiting and schedules elapsed time only for emitted
spans. Later commands use the existing dispatch path. Cancellation, uncertain
restore and pending stop preserve the same confirmed-stop ownership rules.
Masters and enhanced-save bytes remain unchanged. Missing exact-restore support
still refuses the range; no integer rounding or ordinary-start substitution.

This is injected-driver session integration, not a native clock, register driver,
audible output or physical card acceptance. Validation and candidate identity:
`evidence/enhanced-editor/wavetable-range/`.

### Resumable preparation analysis

The renderer now separates synchronous project/PCM validation from incremental
whole-song timeline measurement (`sequence_begin` / `sequence_prepare`). Playback
intervals cannot be requested until measurement succeeds. The wavetable preflight
uses that path and exposes begin/step/close: each step performs one measurement
chunk (at most256 ticks), one next/snapshot, one silent phase chunk (at most256
frames), or one command/capability batch. It retains the first-emitting snapshot
checks and selective source mask. Existing synchronous APIs use the same engine.

Pending results never authorize output. Closing cancels at any stage, releasing
both workspace allocations; steps allocate no additional memory, pin no masters,
and invoke no cache uploads or device callbacks. Options and conversion settings
are copied, while project/source arrays remain borrowed and immutable until close.
The editor preparation owner described below cancels before allowing mutations.

This is bounded traversal work, not a hard UI/real-time latency guarantee: initial
project/PCM validation, metadata scans at setup/reset and PCM checks while creating
commands remain synchronous. The incremental owner integration is described below;
native PLAY remains unwired. See
`evidence/enhanced-editor/wavetable-preflight-step/` for host/native evidence.

### Owned preparation and editor cancellation

Song begin now claims the idle voice owner while returning PREPARING. Every
prepare/next/consume/complete call checks the current sampler generation, project
header/table identities and bridge revision. Playback calls refuse to advance
until preparation is ready. Analysis completes before any master is promoted;
subsequent calls pin at most one selected master each, then transfer the audited
sequence reset for playback without a second whole-song measurement. Ordinary
synchronous open drives the same state machine to completion.

A cancelled or failed preparation releases its workspaces/source pins and clears
its ownership claim. The idle voice owner/bridge stays bound for explicit caller
reuse or close, retaining the outer reservation. Already-promoted unchanged master
copies may remain sampler-owned. Once ready, normal confirmed-stop/cache-detach
rules apply. Failed, incomplete or already-transferred analyses cannot supply a
playback sequence. Only a completed non-mutating sequence may be reset for the
same immutable inputs; unchanged source promotion is permitted.

Editor begin/prepare keeps the pending song in the existing change barrier, so
edit, Stop and disposal cancel before modifying/freeing borrowed storage. The
caller must still close the idle backend before freeing its contexts. No native
PLAY/UI scheduling or card output is enabled. The per-step bridge validation
recorded for this milestone is superseded by the revision-bound guard below;
source promotion still copies a whole sample. Evidence:
`evidence/enhanced-editor/wavetable-preparing-owner/`.


### Revision-bound song ownership checks

An immutable song session now captures the exact bridge, backend and reservation
identities after full initial validation. Every preparation/playback guard compares
sampler generation, project header/table/count, bridge revision and identities,
and checks the current WAVETABLE resource lease using the ownership predicate.
Lost ownership latches backend refusal. These checks do not scan master PCM,
allocate, upload or dispatch voice commands; the ownership callback must remain
synchronous and non-reentrant. Selection is still a harmless UI cursor, and intact
master promotion is allowed without invalidating the session.

Full validation remains at begin, public bridge sync/acquire/location, renderer
setup/reset and source promotion. Borrowed sample/pattern/order arrays must still
remain immutable until close; the guard does not detect unauthorized in-place
writes. This removes repeated scans from the owner guard, not every preparation
or command path, and does not establish a real-time deadline or native card output.
Host instrumentation counts real validator calls without production hooks; the
native fixture checks invalid input, identity replacement, ownership loss and
cleanup with injected callbacks. Evidence: `evidence/enhanced-editor/wavetable-guard/`.

### Validated immutable renderer voices

Ordinary offline renders and opaque immutable sequences now reuse their initial
full project/PCM validation when creating voices, starting independent segments or
changing repeat sources. Private renderer entry points skip only the sample-value
scan: shared descriptor format/rate/capacity checks, playback bounds and alias
checks still run with the same atomic refusal behavior. No sample/cache ownership,
pin or editable master is created by this path. All sources must remain alive and
immutable through completion, including progress/output callbacks.

Public voice and command APIs retain full sample-value validation. Mutable EFx
playback uses the original validating path; it cannot opt into this shortcut.
Host instrumentation proves ordinary sequence command completion does not call
full PCM validation. Tests compare exact 8/16/24-bit loop/segment/handoff results,
check bounded failures and ensure invalid public/private-mutable inputs still fail.
Initial/reset validation, source promotion copies, Studio source acquisition and
backend conversion remain synchronous. No native timing or device claim follows.
See `evidence/enhanced-editor/validated-render-voices/`.


### Bounded master promotion during song preparation

After full input validation and capability analysis, each song preparation call
now reserves one selected master or copies at most4096 PCM/marker bytes into its
private version. The full version counts against the sampler budget from allocation
until cancellation or final release. The project and current-version table are
updated together only when the complete copy is ready. No partial sample or output
is published. Existing immutable versions can be pinned without another copy.

Each step checks generation, table/count/current-version and exact descriptor
identity. Cancellation and stale-source failures discard the unpublished version;
completed masters and active pins retain their normal independent lifetimes.
The internal job requires previously validated immutable inputs and editor/sampler
serialization. It does not replace untrusted input validation, and callers must
cancel before changing/freeing the borrowed source. Public sampler_pin behavior
is unchanged. Initial validation and allocation remain synchronous; this bounds
copy work, not elapsed time or allocator latency. Native PLAY/device timing remains
unfinished. See `evidence/enhanced-editor/bounded-master-promotion/`.

### Studio preparation before output

Ordinary Studio song sessions now analyze the complete sequence without invoking
source providers, pinning masters or producing audio. Begin/prepare advances one
measurement chunk, interval transition or <=256-frame silent phase chunk. The
completed sequence is reset for playback without repeating measurement; the
required-slot mask includes pre-roll, segment and repeat sources. Studio retains
its stereo, true24, ping-pong and slice support rather than adopting mono wavetable
capability limits.

The sampler owner then reserves/copies only those required masters with the same
4096-byte promotion job used by wavetable preparation. All required pins must be
ready before the first output block. Preparation holds pins until stop/end/error,
so later provider acquisitions reuse the immutable versions instead of promoting
whole document-backed samples during a note trigger. Unused slots stay untouched.
Failure or cancellation releases incomplete copies and session pins; unchanged
completed masters may remain sampler-owned. Master precision is never reduced.

Editor begin/prepare and begin_queued keep pending work under the existing
edit/undo/dispose/Stop guard. A queued pump advances preparation with no queued
PCM until ready, preserving independently held consumer blocks on later edits.
The existing synchronous start/open APIs drive the same preparation to completion.
Generation and project-header identity checks reject stale sessions; selected
channel navigation is allowed. Borrowed arrays remain immutable by contract.

Initial/reset validation, allocator calls and public provider/voice validation
remain synchronous. No arbitrary provider gains a trusted-input flag, and this is
not a hard real-time guarantee. Mutable EFx continues its separate private-bank
path. Native PLAY, output scheduling, card transport and physical acceptance remain
open. Evidence: `evidence/enhanced-editor/studio-preparing/`.

### Prepared Studio source acquisition

The ordinary sampler song now supplies its own private source provider over the
pins captured during preparation. Each trigger/segment/repeat acquisition retains
the exact held version after bounded generation, project-header, slot/current-token,
owner and descriptor-identity checks. It performs no project-wide PCM/marker scan,
allocation or copy. Each mixer acquisition owns an independent reference, released
by its existing active/pending-repeat lifecycle before the preparation pins close.
Metadata-only versions continue to retain their shared immutable backing storage.

The internal retain helper requires a genuine held pin and a previously validated,
immutable project; it cannot promote samples or validate untrusted inputs. Unexpected
version/descriptor changes refuse without publishing output or changing references.
This does not authorize in-place edits or freeing borrowed arrays while active.
Public sampler_pin, the general sampler_studio provider and public mixer/voice
validation remain unchanged. The subsequent prepared-voice boundary below also removes selected-source
value rescans for this private path; neither change claims hard real-time
playback. Mutable EFx does not use this provider.

Instrumented host tests count calls to the actual project validator and verify
none occur during prepared playback, including pre-roll and repeat handoffs;
public-provider checks still run and reject invalid unused samples. Native software
fixtures cover version retention/refusal, shared backing and final release. No
native PLAY, card transport, physical timing or audible acceptance is established.
Evidence: `evidence/enhanced-editor/studio-prepared-pins/`.

### Prepared Studio voice establishment

The private prepared sampler-song path now reuses its completed PCM validation
when establishing ordinary voices, independent segments and pending repeat sources.
An internal pull/dispatch/mixer call chain reaches the existing validated voice
helpers only after the sampler owner is ready and its private provider retains the
exact prepared version. Descriptor shape/capacity, playback geometry, format and
alias checks remain; only repeated value scans are omitted. Active/pending pins,
failed-trigger rollback, handoff and final-release behavior use the same code.

No public opt-in flag or persistent trusted-mixer mode exists. Public Studio pull,
dispatch, trigger, segment and repeat APIs retain full value validation, even on a
mixer previously used by an internal prepared call. Arbitrary providers and mutable
EFx never enter this private path. Its contract requires validated immutable PCM,
retained exact versions and serialized owner-thread access; it does not validate
untrusted data or permit writes through master pointers.

Host instrumentation verifies neither project nor PCM value validators run during
prepared direct playback, including pre-roll, slices, offset segments and repeat
handoffs. Both mixer paths retain exact PCM, control, rollback and alias behavior;
public invalid-value tests and private invalid-shape tests still refuse. Initial
validation/reset and allocation remain synchronous. Queued output still performs
its bounded block checks. No native PLAY/transport, real-time deadline, audible or
physical acceptance follows. Evidence: `evidence/enhanced-editor/studio-prepared-voices/`.

### Prepared wavetable source access

Ready wavetable songs now use private source callbacks for live dispatch and
exact range restoration. The callbacks recheck the captured generation, project
header, bridge/backend/reservation identities and live device ownership without
project-wide scans. Acquisition independently retains the exact prepared master
through synchronous upload via pt_sampler_pin_current; changed current tokens or
sample descriptors refuse before upload. Location checks the held cache lease,
validity, captured cache version and live ownership. No source promotion occurs
at a prepared trigger. Required master pins remain held until confirmed stop.

The private dispatch call does not store a trusted mode. Public bridge sync,
acquire/location, manual trigger and public dispatch/restore still use full
validation. Both paths share capability checks, actual-address checks, cache
leases, uncertain start/restore/stop handling and rollback. Editing still requires
stopping the owner; in-place mutation of borrowed arrays is forbidden. No private
callback is an authorization to accept unvalidated sources.

Instrumented host fixtures verify no project-validator calls during ready song
playback, including first uploads, cache-hit retriggers, controls and range
restoration. Changed source capacity, data, metadata and current pins refuse
without uploads; public calls still reject an invalid unused sample even after
private playback. Existing exact range cursor and failure ownership tests pass.
The prepared conversion boundary below also removes redundant selected-PCM
validation. First-use upload remains synchronous with bounded staging/write
chunks, not bounded total trigger time.
Native PLAY/clock, card transport, audible and physical acceptance remain open.
Evidence: `evidence/enhanced-editor/wavetable-prepared-sources/`.

### Prepared wavetable conversion

The ready wavetable song now calls a private cache/upload entrypoint while holding
an independently retained exact master pin. It reuses completed value validation,
including on cache hits, while checking PCM shape/capacity, format, overflow and
staging alias rules. Public PCM sizing/packing/upload and public AmiGUS cache
acquisition always validate values, even after a prepared call on the same cache.
There is no persistent trust flag; mutable EFx banks never enter this path.

Prepared and public uploads share packing, signed rounding/clipping, endian/channel/
padding handling, unpublished lease ownership, publication, eviction and failed
transfer cleanup. Live reservation checks remain at acquisition and every device
write. Conversion reads the master but never changes its precision or contents.
Host comparisons cover all8/16/24-bit source widths and both8/16-bit playback
formats across channel/endian/padding/chunk combinations. Instrumented ready-song
and range tests show no project or PCM value-validator calls during dispatch.

Initial validation/preparation and allocations remain synchronous. A first-use
upload still processes the whole sample with bounded staging/write chunks; this
is not a total-time bound or a physical performance result. Song/editor scheduling
of the incremental jobs below, native PLAY/clock/card transport and physical
acceptance remain unfinished.
Evidence: `evidence/enhanced-editor/wavetable-prepared-conversion/`.

### Incremental playback-cache upload jobs

The PCM upload layer and AmiGUS cache adapter now expose begin/step/cancel jobs.
Begin validates arbitrary PCM (or uses the private prepared-pin contract), copies
its descriptor/format, and takes a cache lease without packing or bus writes.
An existing valid cache hit transfers its lease immediately. A new allocation
returns PENDING and remains unpublished and pinned until completion or cancellation.
Each step converts/writes at most256bytes, limited further by supplied staging and
rounded to complete output frames. Only the final successful write publishes and
transfers the cache lease. Cancel/failure releases partial resources and preserves
caller output leases. Older active representations retain their independent pins.

Each step checks source descriptor identity and the unpublished cache lease.
The AmiGUS adapter additionally checks the captured reservation and live ownership
before each step and before/after each write, including the final write. Detach
cannot free an in-flight lease; cancellation makes cleanup possible. Small steps
retain partial device-word assembly safely until the word or final pad completes.
No pending handle can authorize a playback address or a cache hit.

These low-level jobs borrow source storage and its descriptor: the caller must
keep the immutable master/version pinned through completion/cancel. They do not
acquire a sampler master themselves. Existing synchronous APIs drive the same jobs
to completion and the prepared sampler path retains its source pin throughout.
Song/editor dispatch still calls synchronous wrappers. The sampler-owned layer
below now retains source pins across steps; yielding dispatch remains unfinished.
Initial public validation, cache eviction/allocation and synchronous driver callback
latency remain outside the per-step byte bound. This is not physical timing proof.
Evidence: `evidence/enhanced-editor/incremental-cache-upload/`.


### Sampler-owned incremental uploads

The sampler bridge now provides begin/step/cancel jobs that retain an independent
exact master-version pin and store their PCM descriptor in stable job storage.
This keeps both data and descriptor alive between steps, including after the
caller's separate pin/history references are released. Public begin validates the
project and selectively promotes/pins the requested master. The private prepared
song entrypoint retains an already validated exact pin without allocation or value
scans. Neither begins bus writes; cache hits transfer immediately.

Each step checks captured generation, project header, bridge/backend identity,
cache version and exact current master descriptor before conversion. Selected
channel navigation is permitted. Reservation/live ownership remains checked by
the adapter. Completion transfers only a published cache lease; cancellation and
failure release partial device resources before releasing the master pin. Output
leases remain unchanged on pending/failure. Job storage must not move while active;
owner/context structs must stay alive until completion/cancel. Values remain
immutable, and normal callers cancel before editing or disposing their project.
Cancel itself does not dereference the project and is safe after document disposal.

The existing public synchronous acquire and private prepared-song acquire now
use these jobs. Dispatch still drives all steps synchronously; editor yielding,
pending batch acquisition and timeline scheduling remain follow-on work. Public
begin validation/promotion/allocation and driver callbacks have no wall-time bound.
This milestone does not establish physical AmiGUS or real-time performance.
Evidence: `evidence/enhanced-editor/sampler-upload-jobs/`.


### Yielding song/editor upload batches

The song owner and editor binding expose next_step/complete_step. UPLOADING
means repeat that same operation on a later editor turn; consume and the other
operation cannot advance playback or overwrite the next interval while pending.
The owner captures only trigger slots in this command batch (or active voices in
the exact range-start snapshot). Each call begins one cache acquisition or writes
at most256bytes. A separate final call verifies every selected current master and
cache address, then commits the bounded batch. No new start/restore/control/stop
callback runs during acquisition. Duplicate triggers retain separate cache pins.

Prepared dispatch now receives already-held leases, with no uploads from inside
voice callbacks. Failure on a later acquisition cannot start an earlier voice.
Stop, stale/failure cleanup and the editor edit/dispose barrier cancel unfinished
uploads and release unstarted leases before stopping active voices. Unconfirmed
voices retain their independent device leases and the session's master pins until
stop confirmation. All cache hits and exact fractional range restoration use the
same path. Existing synchronous next/complete calls drive the step APIs to preserve
callers; standalone public dispatch remains unchanged.

This is an owner-thread protocol, not a hardware clock. Existing device voices may
continue playing during acquisition. Native PLAY/event-loop scheduling, lookahead
or deadline handling, card transport and physical timing acceptance remain open.
Initial allocations and the final bounded callback batch are synchronous. Projects
and PCM remain immutable; callers must stop before editing or freeing contexts.
No change to master precision, project saving or the accepted classic display.
Evidence: `evidence/enhanced-editor/yielding-upload-batches/`.

### Immutable lookahead and ahead-of-interval prefetch

A caller-owned Fast workspace can now preview the current pending renderer
interval's upcoming commands without changing live phase or time. Begin copies
command state; each step advances the private copy by at most256frames or resolves
one bounded command plan. Live consume may interleave independently. A monotonically
identified interval prevents stale/rewound jobs from committing. Commit requires
that same live interval to be fully consumed and the preview ready; it transfers
computed command state exactly once. Cancellation changes no live state and does
not dereference the sequence. Mutable EFx sequences cannot use this shortcut.
The renderer workspace borrows immutable sources and does not itself pin masters.

The master-pinned song owner embeds this workspace and exposes prefetch through
the editor binding. After next succeeds, callers can prefetch while the current
interval elapses: preview the upcoming command batch, then selectively acquire its
playback caches. UPLOADING requests another bounded step; OK means ready without
voice callbacks. Unlike after-boundary uploads, live consume remains allowed.
Complete refuses before all current frames are consumed. A ready completion checks
source/cache identity and transfers prepared state/leases without PCM conversion
or cache allocation. If prefetch was not ready, complete_step can continue bounded
work; a future clock driver must decide whether that is already too late.

No whole-project preload, early start, master mutation or precision loss is added.
Stop/edit/dispose cancels preview and upload jobs, then retains any unconfirmed
active playback leases/master pins as before. Silent range pre-roll and pending
range restoration use their existing paths; prefetch starts only once next has
returned an output interval. Initial allocation and final callbacks remain
synchronous. Native event-loop/clock/deadline enforcement and physical throughput
acceptance are still unfinished; these APIs do not establish real-time playback.
Evidence: `evidence/enhanced-editor/lookahead-prefetch/`.

The first lookahead emulator attempt failed at the native test allocator cleanup:
new fixture calloc bypassed malloc remapping while free still used the native
pool. Tests now use matching malloc/zero/release; corrected host tests and native
build pass. Corrected bytes have NOT passed an emulator run. Shared emulator stays
on recovery hold because allocator integrity/clean exit is unverified. See the
lookahead evidence for the original failure and corrected build; a clean restart
requires a new coordinated recovery decision before further guest testing.


### Lookahead corrected native acceptance —27 September2026

Following an explicitly approved shared-emulator clean restart, the corrected
lookahead/editor fixture passed render-files-1790501978750619000 within90seconds:
RC0,1036 Fast allocations fully released, no Chip fallback, all4PaulaDMAoff and
exact staging cleanup. The original failed fixture allocator-mismatch evidence
is preserved separately. Shared030/DevBench is released. See
`evidence/enhanced-editor/lookahead-prefetch/recovered/`. Injected callbacks do
not establish native card access, audio, scheduler deadlines or physical acceptance.


### Strict injected interval-clock gate

An optional song/editor clock gate now owns one positive emitting interval already
returned by next. Arming requires all its frames still unconsumed, computes a
checked absolute deadline in the render sample-rate's frame units, and starts no
voice. While armed, ordinary next/consume/complete cannot bypass it. A service call
before the boundary consumes at most256 elapsed frames plus one bounded prefetch
step. Repeated equal timestamps can finish preparation/phase debt without claiming
more elapsed time. WAITING means the interval is still live, even if its cache is
ready. Manual prefetch remains safe and permitted.

At the exact boundary, the batch must already be ready and remaining phase debt
must fit one256-frame step. Only then does completion validate and dispatch the
prepared plan without allocation/upload. Late service, unready preparation or
excessive phase debt returns DEADLINE and requests stop without dispatching the
late plan. Clock regression or deadline overflow returns CLOCK and requests stop.
Stop cancels unstarted uploads and retains all uncertain active voice leases and
master pins until explicit close confirms their stops. The editor's existing
edit/undo/dispose barrier applies while the gate is armed.

This is deliberately a strict PER-INTERVAL contract with an injected timestamp,
not a complete event loop: zero-frame startup commands, silent range pre-roll,
range restoration and scheduling the following interval still belong to the
caller. Timestamps and callbacks must be owner-thread/non-reentrant, and callbacks
remain synchronous. Native clock reading, callback-duration deadlines, tolerance
policy and sustained physical throughput are unqualified. Native PLAY/output is
still disabled; there is no new classic-layout or sample-master/persistence change.


### Preparing range restoration separately from starting voices

The song/editor next_prepare API now selects the upcoming interval and prepares
any range-start restore leases without voice callbacks. Each pending call retains
the existing one-acquisition or256-byte-copy work bound. Its ready interval is
repeatable without advancing phase/time, allocating again or writing more sample
bytes. Consume, complete and clock-arm cannot advance that staged interval.

A separate next_commit requires readiness and rechecks all source descriptors,
cache leases and device addresses before exact-position restoration. It performs
no sample allocation/upload, invokes restoration once and then exposes the pending
interval. Early/double commit refuses. Stop/edit/dispose cancels staged leases;
stale generation/changed descriptors refuse before restoration. If the restoration
callback itself fails with an unconfirmed voice, existing active-lease/master-pin
retention still applies. Legacy next/next_step wrap preparation and commit and
preserve their prior behavior and output atomicity on failure.

Whole-song zero-frame intervals can likewise be staged without callbacks, but
preparing/committing the interval does not apply its commands; those still use
prefetch/complete. The full scheduler must combine these boundaries with absolute
startup/transition deadlines before native playback is enabled. The new split
commit itself does not check time; it is not a native-clock or physical timing
acceptance claim. Range fractional cursor parity and master save bytes are retained.


### Whole-song scheduling with an injected frame clock

The song/editor schedule_begin/step driver now exclusively owns advancement from
an unopened playback traversal. It accepts an absolute future start frame, checks
that the entire preflight frame bound fits, and primes startup without callbacks.
Each pre-start call performs one bounded stage: interval selection/cache acquisition,
<=256 silent pre-roll frames, one silent completion, or one zero-frame-command
prefetch step. Empty startup commands advance privately; a prepared nonempty batch
waits for the exact start. Range playback retains its staged exact-cursor restore
until that same boundary. Ready returns OK before start; incomplete priming returns
WAITING. Both report the start deadline and invoke no voice callbacks.

At the scheduled start, unready work is refused rather than finished late. The
prepared command or range restore commits, then the first positive interval is
armed. During playback, each call consumes bounded real elapsed phase and prefetches
its upcoming command batch. At each exact deadline, only previously ready work
commits; the following interval is selected and armed in the same call, using that
absolute boundary rather than a new relative delay. The returned next deadline
matches the independent renderer's fractional tick/tempo traversal. Natural end
uses the existing confirmed-stop lifecycle. Legacy advancement APIs cannot bypass
an active scheduled driver; close and editor edit/Stop/dispose remain available.

Regressing time, overflow, late service or missed preparation deadlines poison
playback and request stop. No late catch-up trigger is issued. Unconfirmed voices
retain their playback leases and exact masters until close confirms stop. The
startup overflow bound conservatively includes silent pre-roll. The caller must
provide sufficiently early priming and poll before AND at each deadline; this is
not a sleep/timer implementation. Positive post-start spans rely on the audited
44.1/48kHz supported renderer timeline; unexpected zero/silent transitions refuse.

Tests cover whole/range/lead-in starts, every absolute boundary against a separate
renderer traversal, fractional tempo changes, untouched output on refusal, repeat
readiness, deadline/clock faults and unconfirmed stops. Timestamps and callbacks
remain injected and non-reentrant. Native clock sampling, event-loop wakeups,
callback-duration accounting, device bus binding and physical timing/audio still
require separate qualification. Native PLAY/card output remains disabled.

### Checked elapsed ticks and a sampled-clock adapter

The portable elapsed_clock converts a monotonic64-bit counter at a stable32-bit
frequency to sample frames (rates1..192000). It retains fractional numerator carry
between calls, so polling partitions do not accumulate rounding loss. Splitting
whole seconds and the remainder avoids a delta*rate overflow even when the final
quotient fits. Frame overflow, frequency changes (including zero) and counter
regression/wrap latch failure without changing accumulated time or caller output.
Invalid initialization/overlapping output refuses atomically. Starting a new epoch
is explicit initialization, never an automatic live-playback recovery.

Song/editor clocked_begin samples an injected reader once, establishes frame zero
and schedules a future start delay in frame units. Each valid clocked_service reads
once, advances the checked converter and performs one bounded scheduler step.
Raw timestamp progression cannot bypass a bound reader. Read/conversion failure
poisons playback and requests stop; repeat service does not reread/rebase. Pending
voice stops retain the existing cache/master leases. Editor disposal cancels a
primed range restoration before any output and subsequent service cannot read the
old context. The reader/context must outlive the session and cannot block, reenter,
edit sources or perform voice actions.

Tests use synthetic frequencies, including700001Hz, to exercise fractional carry,
nonzero epochs, large quotients, exact uint64 limits, read/frequency/regression/
overflow/deadline faults, natural completion and uncertain-stop retention. This is
an injected reader adapter, not native clock acquisition or a sleep/wakeup driver.
Deadlines are still frame counts relative to the initial read. Real clock accuracy,
callback duration, event-loop timing and physical bus/audio remain unqualified;
native PLAY/card output remains disabled.

The elapsed clock also has a read-only inverse deadline query: it returns the
first counter tick at or after a requested future frame, accounting for the
current fractional carry and rounding upward. Checked whole/remainder splitting
and borrowing avoid intermediate overflow; unrepresentable ticks and past frames
refuse without changing state/output. The current frame returns the current
observed tick. A clock slower than the sample rate can skip frames: this query
returns a threshold and does not promise an exact-frame observation.

Song/editor clocked_deadline converts the active startup or running deadline
without sampling the reader, preparing work or advancing playback. If that actual
scheduled deadline cannot fit the counter, playback fails CLOCK and requests stop.
Tests check each returned tick reaches the target and its predecessor does not,
compare against an independent original-epoch arithmetic oracle, cover extreme
limits/borrowing, and drive a full song using queried ticks. No timer is armed by
the query, and the existing strict no-late-dispatch policy remains unchanged.

### Native read-only EClock ownership

`src/native/eclock.h` supplies an owner-thread sampled-clock callback using the
native timer.device EClock. It opens its own port/request/unit, requires Exec and
timer.device version36 or later, and keeps the device base local to each read.
It assembles both32-bit counter halves and returns the reported frequency rather
than assuming PAL/NTSC timing. Failed/invalid/closed reads preserve outputs;
outputs cannot overlap each other or the owner. Partial open failures unwind in
reverse order. Close is idempotent and releases the private device/request/port.

The owner must stay open until its reader clients are closed. This request is
NEVER submitted: there is no pending IO, signal wait, clock-setting command,
CIA programming or voice/device-output action. It does not change the existing
editor's display timer and is not yet wired into native enhanced PLAY. Host tests
inject every allocation/open/version failure and check exact release and output
atomicity. The native fixture separately reads the real emulator timer.device
across repeated open/close epochs and feeds those readings through the checked
frame/deadline converter. This demonstrates clock access/lifetime, not wakeup
accuracy, callback budgets, audio cadence or physical timing.

API basis: installed classic NDK devices/timer.h and proto/inline timer headers;
the [AmigaOS timer device documentation](https://wiki.amigaos.net/wiki/Timer_Device)
describes the64-bit EClock count/frequency and distinguishes clock measurement
from timer requests. Modern interface examples on that page are not copied into
the classic68k implementation.

### Native absolute alarm ownership

The separate `eclock_alarm.h` owner opens UNIT_WAITECLOCK with its own port/request.
It accepts absolute64-bit counter deadlines, refuses already-reached deadlines
before submission and refuses rearm while pending, closing or poisoned. A normal
arm returns WAITING; poll calls WaitIO only after CheckIO confirms completion,
collecting each reply exactly once. The signal mask lets a future caller combine
notification with its own event loop, without a wait inside this owner.

Cancel requests AbortIO at most once and then polls for confirmed completion.
A pending close returns0, retaining the device, request and port; the caller must
retain the owner and retry later. No pending request is reused or freed, even if
abort completion is delayed. Device errors poison rearm until close/reopen. A
completed request can be cancelled/collected without abort, and clean close is
idempotent. Partial-open failures unwind. All operations are owner-thread only.

The native diagnostic uses four2ms absolute alarms with explicit Delay(1) polling,
logs observed lateness INCLUDING that coarse caller delay, then cancels a10second
future alarm and checks close/reopen. These are lifecycle/completion observations,
not intrinsic timer precision or playback deadline acceptance. No song is attached
to the alarm and no audio callback is invoked. Actual-time resampling and the
existing strict deadline gate remain required before future playback integration.
The separate read-only EClock owner's request remains permanently unsubmitted.

The signal diagnostic adds16alternating2/10ms alarms. It verifies a distinct,
nonzero signal mask for the alarm and an already-armed separate2second watchdog
before using Exec Wait. Each wake polls both requests and resamples the actual
EClock after collecting the alarm reply. Stale/spurious signal loops are bounded;
the outer shared runner remains the process watchdog if the timer service itself
fails. Watchdog cancellation retains its owner until completion. Measurements
include task switching, poll/reply collection and clock-read overhead; logging is
deferred until the measured cases finish. No timing tolerance or late-dispatch
policy is changed by this diagnostic, and it invokes no playback callbacks.

The27September shared030 signal diagnostic observed193..10045late ticks at709379Hz
(15cases193..198, firstcase10045); all16exceeded one48kHz frame interval. This is
caller-inclusive emulator evidence that the measured task-wakeup path does not
meet the current exact-frame gate. It does not justify relaxing that gate or
claiming physical timing failure. Prepared dispatch/timing integration remains
unfinished; native enhanced/card PLAY stays disabled.

A further native diagnostic primes24-bit sample/cache leases, schedules a200ms
start and drives the production clocked song service from real EClock/alarms with
fake voice callbacks. At frame9656 versus requested9600, the natural wake returned
DEADLINE; an explicitly Delay(1)-delayed case at10778 also refused. Both issued zero
start/control/restore callbacks and released all prepared leases. This validates
native late-refusal/lifetime behavior, not successful real-time playback. The
actual duration of source guards and synchronous dispatch after sampling remains
unqualified and must be characterized before native output integration.


A native duration diagnostic now measures unchanged source guards and prepared
one-/16-voice startup using an injected logical clock and fake callback bus.
Shared030 ready calls took1610..2290ticks at709379Hz (2.27..3.23ms); startup took
10737ticks for one voice and76025ticks for16 (15.14/107.17ms). The16-voice first-to-
last callback spread was35198ticks. Instrumentation and emulator scheduling are
included. Commit allocated/uploaded nothing and all leases were released. These
observations show that readiness and an exact service timestamp do not establish
a bounded realtime dispatch. They do not qualify physical performance or permit
late callbacks. Evidence: `evidence/enhanced-editor/native-cost/`.


### Bounded trigger-command preparation

Prepared song batches now calculate each absolute 8/16-bit trigger command after
all cache acquisitions/uploads finish, with at most one trigger conversion per
preparation call. The fixed-capacity command bank belongs to the bounded song
allocation. Repeated readiness does not recalculate commands. Cancellation clears
its validity together with batch ownership; no command can outlive the song.

Dispatch accepts only the exact private action, rate/format and captured cache
address/byte length, after the existing master/version/cache checks. It copies
the prepared trigger command instead of lowering that trigger again. Whole-batch
capability validation still runs before the first callback, and uncertain-start,
confirmed-stop and lease rollback behavior is retained. Public manual dispatch,
range restoration and control-command calculation keep their existing paths.

Host converter instrumentation checks one conversion per preparation step and
no repeated-ready conversions. For three simultaneous triggers, the commit keeps
three whole-batch validation conversions instead of the previous six total.
This is a bounded preparation change, not removal of all commit-time arithmetic
or native realtime qualification. Native enhanced/card PLAY remains disabled.

The revised candidate passed host sanitizers, staged editor lifecycle tests and
shared030 execution within the separately coordinated120second cumulative suite
window, with1241Fast allocations and zero final ownership. An earlier90second
suite timeout and late RC0 completion are preserved separately with guarded
recovery evidence. The process-budget change does not alter playback deadlines.
Timing observations remain variable and do not prove a speedup or realtime budget.
Evidence: `evidence/enhanced-editor/prepared-trigger/` and `prepared-trigger-timeout/`.


### Prepared ordinary command batches

The private song command bank now covers triggers, controls and stops. Each
preparation call validates one immutable command and computes its trigger geometry
or control rate/gains, after all selected cache uploads finish. Readiness identifies
the exact private batch, rate and format; cancellation invalidates the bank. No
public caller can opt into the prepared path. Public manual dispatch and silent
whole-song capability analysis retain full conversion validation.

At commit, ordinary prepared batches no longer repeat trigger/control conversion.
They still validate the entire live channel/source/held-state transition before
any output callback, then recheck the master/cache/address and uncertain-voice
ownership at each operation. A control whose voice became uncertain, or whose
control callback disappeared after preparation, refuses the whole batch before
any earlier valid control is sent. Failed stop retains its cache/master lease.
Range restoration retains its separate, still-synchronous conversion path.

Host instrumentation verifies at most one conversion per preparation step and
zero conversions at ordinary startup/control commit, with8/16-bit derived plans,
pitch-change parity, repeated readiness, duplicate leases, stale-source refusal,
cancellation and changed active-state tests. Preparing commands does not establish
native realtime deadlines or enable native audio/card output.

The ordinary-bank follow-up passed host sanitizers, staged editor lifecycle tests,
native build/main syntax and shared030 execution (RC0,1255Fast allocations, zero
owned bytes, all DMA off and exact cleanup). Source/ownership checks still dominate
substantial synchronous work; measured startup costs remain variable. This does
not qualify realtime playback. Evidence: `evidence/enhanced-editor/prepared-batch/`.


### Prepared exact range restoration

Range-start batches now calculate at most one active voice's exact byte cursor,
loop bounds, rate and gains per preparation call after all selected cache leases
exist. Inactive channels are skipped within the fixed16-voice bound. The restore
bank belongs to the same bounded song allocation and is invalidated on cancellation.
Repeated readiness does not recompute it.

The private snapshot identity, rate/format and captured address/byte length must
match at commit. All voices must still be idle; master/version/cache checks,
whole-batch acquisition/location validation, uncertain restore ownership and
confirmed-stop release remain in force. Public manual restore retains its full
validation and conversion path. No output callback runs during preparation.

Host converter instrumentation checks one restore conversion per preparation
step and zero during repeat readiness or commit. The range fixture compares
fractional8/16-bit cache cursors with the independent render snapshot and exercises
stale source, cancellation, early/double commit and uncertain-stop lease retention.
Native enhanced/card PLAY remains disabled; this does not qualify realtime output.


The range bank passed host sanitizers, staged editor lifecycle tests and native
cross-build/main syntax. Its coordinated emulator launch timed out without an
acknowledgement; read-only inspection found the guest paused and all DMA off,
with no test log/RC/done. Emulator acceptance remains pending and a recovery hold
retains exact staging/launcher. The runner now refuses a paused/unknown guest
before staging and rechecks before launch; host guard tests pass. No automatic
resume/reset/retry was performed. Evidence: `evidence/enhanced-editor/prepared-restore/`.


The user subsequently approved a single resume of the existing shared guest.
Live CPU/memory/disk/share identity and exact staged candidate were verified;
the original pending launch completed with RC0 and all native restore/editor,
timer/watchdog and ownership assertions passing.1255Fast allocations ended with
zero owned bytes; all four DMA channels were off. Exact archived run/launcher
cleanup and independent absence were verified, and the shared environment was
explicitly released. The original launch timeout remains recorded as failed;
recovered execution is separate evidence and does not qualify realtime/card output.


### Exact snapshot comparisons on native targets

Wavetable and direct-master Studio song guards retain byte-exact project snapshot
comparison, including metadata, padding and tail bytes. Channel-selection remains
the same permitted UI cursor exception. The equality helper copies32-bit words
into unsigned locals with memcpy, avoiding aliasing violations; the68000 path
states the project ABI's two-byte alignment so the compiler emits longword loads.
Other targets use ordinary alignment-safe memcpy. Every original comparison site,
generation, master/cache and ownership check remains; no checksum or revision-only
shortcut replaces snapshot validation. This compares the descriptor object, not
borrowed arrays or PCM, whose existing immutability contract remains unchanged.

Host tests flip every byte in both directions, check equal/self-alias cases and
restoration without modifying inputs, including native two-byte ABI alignment and
the final short tail. Existing Studio, wavetable and editor stale/lease tests pass.
Generated68000 source guards use376longword comparisons plus two trailing bytes
instead of a1506byte loop. Native performance observations remain separate from
realtime or physical acceptance.


The exact snapshot change passed shared030 execution (RC0,1255Fast allocations,
zero owned bytes, timer/lease closure, all DMA off and verified cleanup/release).
Observed readiness748..757EClock ticks and startup4440/26989ticks for one/16voices
at709379Hz are lower than the preceding observed costs, but remain instrumented
emulator measurements, not realtime/physical acceptance. Evidence:
`evidence/enhanced-editor/snapshot-equality/`.


### Finite native timing diagnostic

`tools/build_editor_wavetable.py --timing-only` builds the no-argument
`PTExecWavetableTimingTest` with its own manifest. It runs clock/alarm/watchdog,
late-service refusal, 1/16-voice simulated cost and exact snapshot fixtures, then
verifies all Fast allocations and leases released. It does not access audio or
AmiGUS. `tools/shared_infra_timing.py` qualifies it using the existing shared
core runner and exact-byte promotion gate via a process-local profile; installed
shared configuration stays unchanged. The default full editor test is unchanged.

The 259392-byte candidate passed shared030 run `20260927T120335293381Z`, RC0,
29 Fast allocations/zero owned bytes, all DMA off, exact cleanup and explicit
release. Physical promotion accepts those exact bytes. Physical execution and
realtime acceptance remain separate. Evidence: `evidence/enhanced-editor/finite-timing/`.


The separately invoked `tools/shared_infra_timing_physical.py` caller requires a
coordinated, user-authorized physical/DevBench window. It checks shared exact-byte
promotion before selecting hardware, delegates CPU/network/bridge identity and
RAM-only upload/execution to the existing shared physical runner, validates all
completion/resource markers and idle DMA, and deletes only its exact three owned
files and directory after success. Every selection attempt is paired with emulator
restoration and live verification in `finally`. Failure retains diagnostic evidence
and guest files; there is no retry, reset, install or network configuration change.
Host tests cover refusal before selection and restoration on selection, execution
and cleanup failure. These host guards do not establish physical availability.


### Integrated editor Studio output owner

`pt_editor_studio_output` composes the editor's cancellable master preparation,
24-bit producer, bounded queue and injected PCM FIFO session. A serialized caller
can attach, start, step, stop and detach through one lifecycle. Queue capacity is
1..8 blocks; each step performs at most one producer operation and one output
operation. Capacity stalls preserve queued data. Natural completion drains and
resets before releasing the queue. Editor changes/undo/disposal and output errors
stop source production; pending or failed reset retains the queue and forbids
restart/detach until confirmed cleanup. Initial reset failure follows the same
recovery contract. Errors remain visible after cleanup until a new start.

The owner does not discover/reserve a device, install interrupts, access MMIO or
wire native PLAY. Port and drain contexts, including any external reservation,
must outlive successful detach. Host sanitizer fixtures compare every packed
24-bit byte against a direct-mixer reference and cover startup refusal, stalls,
edit/undo/Stop/dispose, partial-write/capacity/source faults and reset recovery.


The integrated output owner also accepts an already-open PCM reservation. Its
single access lease and queue/session storage survive reset until a separate
bounded `quiesce` callback confirms all adapter references/interrupts removed.
Queue/session cleanup now follows quiescence, before access release. Pending/failed
quiescence refuses restart, detach and card/library close. The caller retains the
reservation after the access lease ends. This is an ownership contract for an
injected adapter, not capability validation or permission for MMIO. Host and native
shared030 fixtures cover natural completion and failed startup/Stop/reset/quiescence;
141 Fast allocations return to zero. Evidence:
`evidence/enhanced-editor/editor-studio-reserved/`. Native PLAY remains unwired.


### Broad committed-source regression,27September2026

A clean export of1ac4856 builds139native executables, including the editor. All
200host tests now have passing execution:198passed in the exported-suite run,
while two Git-dependent source-export tests required separate successful runs
through their existing repository-root helpers. The initial setup errors are kept.
Fourteen registered noninteractive core cases and five Paula/cache fixtures passed
shared030 with exact candidate hashes, all DMA off and confirmed cleanup/release.
Paula's CIAA fallback subcase remains unexecuted because Workbench owns timerB.
The139builds are not139runtime qualifications; actual AmiGUS, native Studio PLAY,
physical performance and listening remain separate. See evidence folders
`core-regression-20260927/` and `paula-regression-20260927/`.


The same committed editor candidate also passed a shared030 UI workflow: cancelled
requester, exact24-bit WAV/two stems, master-preserving bounce and verified save,
undo/redo and clean exit. Recents settings were isolated/restored byte-for-byte;
all DMA was off and cleanup/release verified. The classic layout was inspected.
Evidence: `editor-regression-20260927/`. This exercises native file workflows,
not Studio device output.

### Studio storage survives callback shutdown

Queue/session storage now remains valid after device reset while adapter
quiescence is pending or uncertain. Exactly1 plus a cleared interrupt guard is
required before freeing the queue and detaching the session; the access lease
ends afterwards. An adapter reporting success cannot override a live interrupt
guard. Regression fails against the prior ordering; host and native fixtures
pass, including direct24-bit parity and141 Fast allocations returned to zero.
Evidence: `evidence/enhanced-editor/editor-studio-quiescence/`. These injected
callbacks do not qualify real IRQ removal or device silence.

### Complete regression,28September2026

An immutable87c3833 source export passes all201 host tests (475.074s, exit0) and
builds139 native executables. All288 listed source hashes and139 binary hashes/
lengths were independently verified. Separate Git-test index/worktree settings
avoid including unfinished display work. Later PCM drain-status fix1ed99ac has
its own13 focused host checks and seven-case native fixture; it is not included
in the older full-build snapshot. Evidence:
`evidence/enhanced-editor/complete-regression-20260928/`. Physical/device-output
acceptance remains separate.


## Native crash-recovery integration

The native editor now uses the bounded recovery store and scheduler. Configuration
is opt-in through these Amiga global environment variables, read at startup:

| Setting | Accepted value |
| --- | --- |
| `PT24G_RECOVERY_DIR` | Existing recovery directory; canonicalized through AmigaDOS. |
| `PT24G_RECOVERY_MEDIA` | `fixed` or `removable`; explicit user classification, not automatic hardware detection. |
| `PT24G_RECOVERY_SECONDS` | 30 through 86400; default 300. Zero disables recovery. |
| `PT24G_RECOVERY_REMOVABLE` | Must be `1` in addition to media classification to permit removable-media writes. |

Missing/unknown media, invalid paths/intervals and unapproved removable storage
leave automatic writing disabled. No preferences panel is provided yet. The app
reads these values; it does not modify them. A safe fixed-volume configuration can
use a dedicated existing directory, avoiding repeated floppy writes.

Snapshots run only while the native audio device is closed and sequenced playback
is inactive. An unchanged successful revision is not repeatedly written; ordinary
interval/backoff and clean/undo behavior use the previously tested scheduler.
The native adapter canonicalizes source paths and uses stable source identity.
AmigaDOS FileInfoBlock buffers are explicitly longword aligned. Session directory
names fit 29 characters, and read-only startup/load discovery inspects at most 128
root entries; an incomplete scan reports failure rather than selecting silently.

A matching newer valid snapshot prompts Recover or Keep current. Recovery loads
into a staged document and editor before replacing the open song, preserves all
master precision, strips recovery-only metadata and marks the result unsaved.
Save writes a new verified project; it never overwrites the original source as a
recovery mechanism. Save/new/load/confirmed quit discard only snapshots owned by
the current live session. Previously abandoned crash directories remain untouched;
they are never silently adopted or deleted. An older source reopened later may
therefore offer its retained recovery copy again. Failed cleanup disables further
writes instead of repeatedly attempting them every editor tick.

`evidence/enhanced-editor/recovery-native/` records native configuration/traversal,
365 Fast allocations returned to zero, clean-state discard and later undo resnapshot.
The fixture's intentional assertion probe now exits normally and frees its tracked
Exec blocks before the functional run is allowed to start. This covers the fixture's
tracked allocations, not arbitrary corrupted-program recovery.
`evidence/enhanced-editor/recovery-ui/` records visible decline/recover choices,
real 30-second idle autosave, exact restored project save, unchanged original master
records and normal editor exit. Five temporary test settings were restored exactly,
and each guest window was independently checked and released. The seed helper
created recovery state without crashing the machine. Actual crash/reset endurance,
physical storage failure/performance and real AmiGUS hardware remain separate gates.

The final native recovery editor candidate is 262480 bytes, SHA256
`01641ef5e61d469004cafd772985c333b90772eae5ffe605dbb31a961c06a166`.
Its final shared030 run is recorded in `recovery-ui/final-candidate/`. Amiberry IPC
captures can be black while the guest requester is visible; fresh AmigaBridge
bitmaps were inspected before both final requester inputs. The final status text
fits the existing field. The complete host run passed 200 checks; two helpers
failed in a non-repository export and passed their corrected-environment reruns,
giving passing results for all 202 checks without claiming the initial invocation
was wholly successful.

## Recovery setting read failures

Recovery now distinguishes a missing optional setting from an empty, unreadable
or incomplete value. Only an absent interval takes the 300-second default.
Intervals contain decimal digits only; removable permission is absent, `0` or `1`.
All settings reject embedded controls and potentially truncated buffers before
opening the recovery directory. Binary environment reads prevent a line break
from hiding a suffix. One spare buffer byte is required conservatively across
AmigaDOS return conventions; this bounds directory input to 358 bytes.

[The AmigaDOS GetVar contract](https://developer.amigaos3.net/autodocs/dos.library/GetVar.html)
permits successful truncated reads and changed its length return convention after
V36. The new host fixture exercises both conventions, missing defaults, read
errors, embedded NUL/line breaks and explicit removable policy under ASan/UBSan.
The native private-input cumulative fixture also passes with 365 Fast allocations
returned to zero. These tests neither modify guest ENV nor establish physical
storage behavior. See `evidence/enhanced-editor/recovery-config/`.

The hardened editor (262628 bytes, SHA256
`1ecaee823ff9829762c1ec7a6f2ffeda0c1a0ef2dfae3152b65fac5d7c81c552`) also passes
actual AmigaDOS settings, offer/decline/restore, idle autosave and exact save in
shared030. Evidence is in `recovery-config/editor-ui/`; all five temporary ENV
values were restored exactly before independently verified cleanup and release.

## Wavetable callback quiescence

The voice owner can bind an explicit adapter-quiescence callback before playback.
Closing first confirms every held voice has stopped reading sample RAM. It then
calls the barrier once per close attempt. Pending, failed and unknown positive
results preserve the bridge/cache/reservation and keep editor edits and disposal
blocked. Exactly 1 is accepted only with the reservation's interrupt guard clear.
No repeated voice-stop calls occur after those voices have acknowledged stop.

The same barrier applies to cancelled preparation before the first note, because
an adapter may already retain callback contexts. Master pins and the song owner
remain until the barrier completes. Existing purely synchronous injected drivers
may omit the callback and retain their prior preparation-cancellation contract.
A future native asynchronous adapter must supply the barrier; this change neither
installs interrupts nor claims a hardware quiescence implementation.

Host sanitizer regressions cover 16 active voices, cache retention with zero voice
pins, pending/error/unknown acknowledgements, a still-held interrupt guard, and
editor import/edit/disposal refusal through both playback and preparation stops.
Native emulator acceptance is recorded separately in the checkpoint/evidence.

For adapters with this asynchronous barrier, synchronous song open is refused
before work. Callers use begin/prepare, which publishes the song handle before
preflight can fail. A forced frame-limit failure verifies that the published
handle, callback context and editor barrier survive until cleanup confirms idle.
This preserves the synchronous API's existing failure-without-ownership contract.

The final cumulative shared030 fixture also passes: 420216-byte candidate
`5ed539aedf12e108ed927df67e1c53478037389b0014045eec5a3d9875891bdd`,
1265 Fast allocations returned to zero, including failed preflight followed by
pending cleanup. Independent cleanup and explicit emulator release completed.
See `evidence/enhanced-editor/wavetable-quiescence/`. This is injected voice-bus
acceptance, with separate native timer diagnostics, not AmiGUS hardware output.

## High-resolution masters through the classic Paula song bridge

Otherwise classic-compatible four-channel Paula songs can now play mono16/24-bit
masters through private rounded8-bit playback copies. Rounding uses nearest with
ties away from zero and saturation, without dither or resampling. The authoritative
PCM, sample descriptors and enhanced save remain unchanged. Strict lossless MOD
export still refuses precision loss; the existing explicit export policy remains
separate from this temporary playback representation.

Only referenced instruments receive Chip allocations. Restart advances the cache
generation and refills storage from the immutable master, including after EFx
modifies a private copy. Stop releases the cache. Pattern/control synchronization
uses the same rounded representation as startup; the editor sample-generation
barrier stops playback for any sample edit, including low-bit changes whose
derived8-bit bytes would be identical. Direct adapter callers must retain this
sample-revision barrier: streaming sync checks output compatibility, not exact
master identity. Stereo, nonclassic rate, slices, unsupported loops, Studio mode,
AmiGUS/MIDI routing and other classic limitations remain explicitly refused.

Host sanitizer checks cover8/16/24-bit preflight, full-export versus bounded-stream
parity, conversion boundaries, source preservation and refusal of other features.
The native cumulative Paula fixture passes in shared030: actual audio.device/CIA
playback, Chip/Fast placement, unused-cache omission, restart refill, changed-cache
stop/rebuild, exact enhanced saves and partial-allocation cleanup. All five native
fixtures returned0. CIAA fallback execution was not run because Workbench owns
timerB; existing ownership was preserved. Independent cleanup confirmed all DMA
off and exact run/launcher absence before explicit release. No physical testing
or listening acceptance is implied. See `evidence/enhanced-editor/paula-highres-song/`.

The full native editor also passes the shared030 UI workflow: mixed16/24-bit
song playback, low-bit-only24-bit sample reverse stopping DMA even when derived
8-bit bytes match, exact undo, pattern restart, save, reopen and second exact save.
Candidate262564 bytes SHA256
`83353fbdf697635278e53bbbcaedc5f76c45a984663b496c172df567daa5df2d`;
run `paula-highres-ui-1790559767477699000`. Both normal exits, exact temporary
settings restoration, independent cleanup and explicit release passed. An earlier
UI runner queried DMA before acknowledging Stop; that failed run is retained
separately and the corrected runner waits for a fresh stopped application frame.

## One-to-three-track Paula song views

The native Paula bridge now pads1/2/3-track projects into private four-track
replay rows. The project retains its original track count, packed event layout
and sample masters; enhanced saving is unchanged. Padding uses the bounded
Fast-RAM workspace owner and is released after replay stops. Padding allocation
or preflight failure leaves existing playback intact; failures after replacement
starts use full-stop cleanup. A track-count change stops playback
and requires restart; pattern/control sync reuses the same private workspace.

Unused tracks become silent default Paula slots in the private view, including
clearing inactive saved MIDI endpoints there only. No active AmiGUS/MIDI route
is discarded: unsupported active backends remain refused. Physical voice masks
use the original active track count. Classic four-track projects need no padding
allocation. Native cumulative shared030 tests pass allthree counts, Fast placement,
unused-voice silence, future-row edits, count-change stopping, exact source save
and final allocation/cache release. CIAA fallback remains not run while owned
by Workbench. See `evidence/enhanced-editor/paula-short-song/`.

The263252-byte native editor also passes a three-track16/24-bit shared030 UI
workflow: onlythree DMA voices active, low-bit sample edit stops, undo, pattern
restart, exact save/reopen/play/second save. Native guest bitmap confirms the
classic layout and empty fourth column. Both normal exits, fiveENV exact restore,
independent cleanup and explicit release passed. Evidence is under
`paula-short-song/ui/`; this remains emulator-only acceptance.

## Selective Paula song preparation — 28 September 2026

Native song start and live comparison now build a fixed-size private sample view
before export. All stored patterns are scanned, including instrument-only events
and patterns outside the order list. Referenced sample numbers1–31 retain their
original identities. Unreferenced descriptors and preserved sample headers become
safe empty entries only in that view; their PCM is omitted from both immutable
Fast staging and optional Chip playback caches. Projects can therefore store up
to255 masters, including unused stereo24-bit/48kHz masters, without those unused
samples blocking an otherwise compatible Paula song. All source masters, slot
numbers and enhanced save bytes remain unchanged.

This does not remap referenced slots32–255: they are explicitly refused. Referenced
stereo, nonclassic rates, slices and unsupported loops/routes still require their
appropriate backend. The entire source remains validated, and unsafe preserved
one-word DMA loop metadata remains refused even when its sample is unused.
Unknown optional extensions are not silently discarded. One-to-three-track
padding continues to use bounded Fast workspace; four-track songs need none.

A refused start preserves the existing replay. A newly referenced unsupported
sample stops live sync and releases ownership before restart. Supported changes
to the working set likewise require a stopped rebuild. The editor's sample
revision barrier remains conservative and stops even for an unused master edit;
the lower-level sync comparison checks the derived selected representation.

Host sanitizer checks and cumulative native shared030 execution pass. Full editor
UI qualification also passes with a3-track255-master fixture: playback, low-bit
edit-stop, exactundo, restart, byte-exactsave/reopen/play/second save. Classic
layout inspected, bothnormal exits and allfive temporarysettings restored. Owned
run/launcher cleanup independently verified before explicitrelease. Candidate
263848 bytes SHA256`db6c7145f3b6ed1785b4f200bf7c598631ec1362c4899e0c5d0276f23231f510`.
See `evidence/enhanced-editor/paula-selected-samples/`. Physical remains untested.

## High source-slot mapping for Paula

The private song view now maps referenced source slots32–255 into unused replay
slots. Selected source slots1–31 keep their original numbers; higher source IDs
are assigned in ascending order to the lowest free replay slots. The working set
is bounded to31 distinct referenced masters across all stored patterns. More than
31 is explicitly refused, preserving an already playing song. This supersedes
the earlier low31-source-slot limitation, not the current one-to-four-track,
mono/classic-rate and effect compatibility requirements.

Mapping and short-track padding share one bounded Fast event workspace. Ordinary
four-track songs using only low slots retain the allocation-free event-view path.
Sample descriptors and PCM remain borrowed read-only while the private copy is
encoded. Original CMOD headers are preserved for their original selected low
slots; high masters receive generated playback headers. Saved project events,
source IDs, master data and optional extensions are never rewritten.

The running owner stores the31 source identities independently of derived PCM.
Live sync stops and releases playback if any identity changes, including a switch
to a different source with identical PCM and metadata. The next start advances the
cache generation and reloads its private data. Insufficient mapping workspace,
unsupported referenced formats and excess working sets never replace active
playback on a failed start. Host ASan/UBSan tests cover allfour track counts,
low-slot holes, high32/255 IDs, unordered/instrument-only events, exactly31 versus
32 distinct references, exact source preservation and identical-PCM identities.
Native cumulative shared030 acceptance passes all5fixtures in66.50seconds under
an explicit90-second bound. The earlier60-second deadline failure is preserved
separately despite normal late completion. Full-editor functional acceptance also
passes: high32/255 playback, conservative unused-sample edit-stop, exact undo,
pattern restart and byte-exact save/reopen/play/second save. Both normal exits,
fiveENV restoration, independent cleanup and explicit release verified.

Two screenshot-client attempts timed out at20/45seconds; both remain FAILED and
unresolved. Their capture service later saved images and normal cleanup completed.
The separate functional-only UI run made no capture attempt and grants no visual
acceptance. Candidate264512 bytes SHA256
`1324c3f6a4415cd77d3f65a8d1ae0d057d4abecb2060bedf1e20c42aad2b320b`.
See `evidence/enhanced-editor/paula-mapped-samples/`. Physical remains untested.

## Odd-frame Paula playback copies

Selected mono8/16/24-bit masters may have odd frame counts. The private Paula
encoder rounds precision when needed and appends one zero byte after each odd
master, including that byte in its replay header length. It allocates no second
int32 PCM master or padded master buffer. The original frame count, loop points,
precision and saved PCM remain unchanged. The bounded live comparison streams
the same padded representation and still stops on changed derived sample bytes.

The maximum remains131070 playback bytes per selected sample; odd131069 fits,
odd131071 does not. Loop points still require safe even-word coordinates, and
unsafe preserved CMOD one-word loops remain refused against the original source
length. Padding does not make an unsafe loop valid. Strict MOD exports retain
their existing odd-length refusal; playback permission is a separate API and
cannot be selected through the public direct/round8/TPDF export policy.

Host ASan/UBSan checks pass across allthree precisions, one-frame and multi-block
boundaries, maximum lengths, silent tails, encoder/stream byte parity, bounded
sync, alias/capacity guards, sink failures, source preservation and unchanged
strict export behavior. Native/editor qualification is recorded separately in
`evidence/enhanced-editor/paula-odd-samples/`.

Cumulative native acceptance passes all5fixtures in69.25seconds. New native cases
verify five-frame8/16/24-bit masters, six-byte Chip copies, silent tails, live
sample-change invalidation and exact saved sources. An initial test observed the
first note too early and remains a separate failed run; bounded first-row
observation fixes the fixture. Full editor functional-only qualification passes
three-track255-frame16/24-bit play/edit-stop/undo/restart/exactsave/reopen/second
save. Both normal exits, fiveENV restore, DMAoff, independent cleanup and explicit
release passed. No screenshot/visual acceptance is claimed, and previous capture
failures remain unresolved. Native editor265184 bytes SHA256
`badc183fbecf3111a8f08fc9801a02e923dca7f74dd4e08929b3bf1cc51c9969`.

## Stopped-state visual qualification

A separate runner option now captures only after a fresh STOPPED - AUDIO RELEASED
frame and allfour DMA channels off. The odd two-master editor and the exact older
255-master mapped candidate both pass complete640x512/four-plane/2048-row captures
before and after reopening, each in about6seconds. Allfour bitmaps retain the
accepted classic layout. Functional play/edit-stop/undo/restart/exactsave/reopen
also passes, with normal exits, exact ENV restoration and independent cleanup.

The mapped comparison uses the same editor and fixture bytes as the earlier
active-playback capture failures. Those failures remain recorded and unresolved;
the new stopped-capture workaround is not active-animation, audio-quality or
performance acceptance. No product/display/shared-bridge changes were needed.
See `evidence/enhanced-editor/paula-stopped-capture/` for each evidence tier.

## Integrated host regression sweep

All168 host test modules pass on the isolated odd-frame candidate sources. Two
Git-dependent checks initially failed to locate archived source paths from the
exported tree; they pass when rerun using their existing committed/indexed
checkout isolation helpers. The original launch errors are preserved separately.
Source/test/helper comparison confirms the tested product sources match04d1f74.
This includes cache/master ownership, sample processing/history, direct24 Studio,
renderer/oracle comparisons, persistence, recovery and guard/refusal checks.
It does not establish native device output or physical acceptance. See
`evidence/enhanced-editor/sample-memory-host-suite/` for the complete records.


Studio PCM output now has an optional injected prefill/start acknowledgement gate:
`pt_amigus_session_open_started` and `pt_amigus_register_start`. It preserves queue
ownership through pending/failed enable and requires confirmed reset before reuse.
This extends the software transport only; it does not enable native AmiGUS MMIO
or establish card playback acceptance. See `AMIGUS_STUDIO_TRANSPORT.md`.


## Studio start, prefill and integrated regression

The injected PCM session can now enable only after capacity-bounded prefill and
exact device acknowledgement. Its optional editor binding preserves edit/undo/
Stop/dispose and PCM reservation ownership through uncertain startup and reset.
Short/empty streams cannot deadlock waiting for the configured prefill. Native
register binding, real device FIFO/readback and sustainable service timing remain
unqualified; these controls do not wire native Studio PLAY.

Latest source8cefe39 builds139 Amiga executables with298 source hashes verified.
All203 host cases qualify (one HEAD-based reference rerun on8cefe39), plus two
production-Exec Studio fixtures and14 registered native core cases. Every run
exited normally, released owned resources, verified DMAoff/exact cleanup and was
explicitly released to AmiConnect. Native editor/Paula bytes match their prior
qualified candidates. Evidence: `evidence/enhanced-editor/studio-prefill/`.
Physical machine stayed off and untouched.


## Bounded synthetic recording staging

`capture` collects negotiated8/16/24-bit mono/stereo PCM into a caller-budgeted
allocation made before input starts, using the supplied allocator (the native
master pool can supply bounded Fast RAM). Each append validates and copies at
most256 frames without allocation, conversion or truncation. An invalid chunk or
explicit/device-reported overrun blocks publication; overrun remains a distinct
sticky result. Failed chunks leave the captured prefix unchanged. Empty or active
recordings cannot become samples.

`sampler_capture` appends finished data as one undoable new sample through the
existing sampler version transaction. Success deep-copies all precision into the
sampler budget then releases staging. Failure preserves recording/project/history
for retry. The temporary recording and sampler copy coexist during publication,
so the shared native allocation ceiling must cover both. Existing masters are
never replaced. The caller must apply the editor change guard before publication.

This is software staging with synthetic input, not an AmiGUS recording backend.
Format negotiation, actual device input, reservation/interrupt ownership, overrun
measurement and native recording UI remain required. Buffer close does not stop
hardware; no device may DMA into or retain the collector allocation. Accepting24
bits here does not prove24-bit capture support on Mini or Zorro. The pinned driver's
capture formats must be reconciled with actual variant capabilities before use.

### Injected recording lifecycle

`capture_session` adds a serialized, caller-owned input boundary around the
collector. It allocates before input starts, negotiates no hardware capability,
and performs at most one bounded adapter callback per poll. Input copies at most
256 frames synchronously into scratch storage; adapters cannot retain that pointer
or DMA into it. Exact sample values go into the existing master staging path.

Pending/failed starts, read faults, explicit device overruns and cancellation all
require a confirmed stop. Exactly `1` means disabled, callbacks quiescent and no
retained references; `0` remains pending and every other stop result is a fault.
Faults remain sticky, but stop polling can still complete cleanup. Until that
confirmation, close and recording transfer both refuse and allocations remain
owned. The caller must schedule bounded polls and retain the session/context;
a stop deadline cannot safely turn an unknown device state into freed storage.

A full frame budget finishes automatically; explicit Finish keeps the recorded
prefix. Abort or any fault discards only after confirmed stop. A successful,
nonempty, stopped collector can be moved without allocation into a separate owner
and published through `sampler_capture`; publication failure retains it for retry.
The fixture covers all six sample formats, delayed start/stop, unknown acknowledgments,
malformed counts/values, overrun, cancellation, zero owned bytes, and undoable
publication with low 24-bit bits intact. Host sanitizer and native Exec
checks pass; see `evidence/enhanced-editor/capture-session` for exact scope.

This is an injected software adapter, not a native AmiGUS recording backend.
Actual format negotiation, variant capabilities, hardware reservation/interrupts,
input register binding, native editor controls and measured capture quality remain
unimplemented or unaccepted. No physical hardware is exercised by these fixtures.

### Recording PCM reservation guard

`amigus_capture` holds an existing reservation's exclusive PCM access lease for
the recording session. The pinned SDK has a PCM-block flag covering playback,
recording and mixer; it does not expose a separate recording reservation flag.
A concurrent Studio/capture owner is therefore refused. Duplex sharing requires
an explicit combined owner and is not implemented by this guard.

Input cannot start/read/stop through this owner without the held PCM lease.
Open validates callbacks and acquires staging before starting input; allocation or
format failure unwinds the unused lease without touching the device. Capture
faults/cancellation retain the lease through stop. A claimed stop success while
`reservation.interrupt` remains set is treated as pending, so the collector, context
and reservation stay alive until interrupt removal/quiescence is also confirmed.
Only then may a stopped recording be transferred and the caller release its card
reservation/library. This is injected software ownership, not native recording,
actual input capability negotiation, hardware MMIO or physical acceptance.

### Editor recording ownership and publication

`editor_capture` attaches to the existing veto-capable editor change barrier,
refusing any other active guard. It uses the editor sampler allocator for bounded
recording staging and the exclusive PCM capture owner described above. Each poll
performs bounded input/shutdown work; no input callback runs inside an edit guard.

Edit, undo or disposal requests Finish and remains refused until the device and
interrupt are quiescent. Nonempty stopped PCM remains owned and continues to veto
mutation/disposal until explicitly published or discarded. Publication appends one
undoable master, selects it and refreshes sample bounds. Allocation failure retains
recording data, project/history and selection for retry. Empty/cancelled/faulty
recordings are not published. Faults remain visible after cleanup until explicit
discard or a new successful start. Detach explicitly discards and refuses while
asynchronous shutdown remains unresolved; editor and allocator contexts must live
until it succeeds.

Host and native Exec integration tests cover actual editor note/undo/disposal
paths, navigation, delayed and invalid stop acknowledgments, publication failure,
exact low24-bit data and undo/redo. See `evidence/enhanced-editor/editor-capture`.
This owner is not wired to native recording controls and has no actual input
backend. The accepted classic layout and unrelated display work are preserved.

### Recording integration regression — 28 September 2026

After the collector, recording lifecycle, PCM/interrupt lease and editor barrier
increments, all147 native executables build and all207 host cases in172 modules
pass. The previous139 binary hashes are unchanged; all eight added fixtures are
built, and the four Exec-backed recording variants separately pass the shared030
emulator with zero owned bytes. The full manifest now verifies350 source inputs
and a separately generated font header.

Additional emulator checks of the exact full-build sample dispatcher, RAW/WAV
importers and project-stream writer all pass with bounded workspace, exact master
precision and zero owned memory. Each is independently cleaned and the window
explicitly released. See `evidence/enhanced-editor/recording-integration` for source
attribution, full host logs and per-fixture scope. This is software/file/lifecycle
acceptance; actual recording devices, native recording controls and physical
AmiGUS capabilities remain separate gates.

## Exact recording input format boundary

The injected input binding now carries its exact, already-negotiated precision,
channel count and rate. A missing, malformed or mismatched tuple is refused before
collector allocation, input callbacks or acquisition of the PCM access lease.
There is no implicit 24-to-16-bit fallback, resampling or channel conversion.
The session copies the selected tuple; start must still acknowledge that exact
format and use the existing stop/quiescence failure path if the binding is stale.
Editor refusal preserves the project, history and selected sample.

This is a software contract enforced against injected adapters. Actual hardware
capability discovery/negotiation and a native input adapter remain required.
The collector's support for 24-bit PCM does not advertise 24-bit device capture.

## Recording publication without a second PCM allocation

When finished capture staging and the sampler share the same allocator functions
and context, publication transfers the unique PCM allocation into an immutable
sampler version. The whole allocated capacity, including unused recording space,
is charged to the sampler budget. Publication does not compact or silently reduce
the master. Different allocators retain the transactional deep-copy path.

A failed budget check, allocation or history commit leaves the recording available
for retry. Successful publication clears capture ownership. Metadata versions,
undo/redo and playback pins retain the transferred buffer until the final reference
is released; the sampler and allocator context must outlive those pins. No device
may still own or write capture staging when this operation is called.

Host validation and native build results are recorded separately in the capture
transfer evidence. Emulator execution is pending shared-030 availability; this
change does not enable real input or qualify physical recording.

## Sampler undo-record accounting and retained recording pins

Sample-edit undo records now participate in the sampler byte ceiling, alongside
immutable versions and expanded sample tables. A record that does not fit is
refused before allocation; rollback, history eviction and redo truncation subtract
its charge exactly once. Previously those records used the bounded native pool
but were omitted from the sampler's narrower budget.

An allocator-observing regression reproduces the previous accounting mismatch
and checks a one-byte-short refusal without changing the master or history.
Recording ownership tests also remove an appended slot through undo, reuse it
with a new 8/16/24-bit recording, discard redo history and evict undo entries while
old and current playback pins retain their respective buffers. Stale sampler
generations refuse new pins, and final release returns all sampler-owned bytes.
These host checks do not establish native voice dispatch or physical playback.

## Routed Paula cache owner (software seam)

`sampler_paula` adds a dedicated optional cache for explicitly selected samples
on any Paula-routed track among channels 1–16. Project validation rejects a fifth
Paula assignment. Bind allocates nothing; acquisition pins the current immutable
master during conversion and produces only signed 8-bit, even-byte-padded copies.
Stereo requires an explicit source-channel choice. Master precision and sample
rate remain unchanged; no implicit downmix or resampling is performed.

The caller supplies the storage allocator: native callers must use Chip RAM.
Sampler revisions, undo, sample-table changes and route changes retire cache
copies. An active lease keeps its old bytes until confirmed stop and unpin; it
cannot authorize a new trigger. Memory pressure evicts only unpinned copies.
Busy close blocks new acquisitions and retains active data until readers stop.

This owner is tested with injected storage and has an Exec fixture that checks
Fast-RAM masters and Chip-RAM cache allocations when executed. It is not wired
to native replay yet. Mixed-backend scheduling, dynamic physical voice dispatch,
geometry/capability checks and end-to-end playback remain required. The existing
four-channel replay path and its refusal behavior are unchanged.

## Routed Paula voice ownership (software seam)

`paula_voices` owns four injected physical slots using the existing stable
`pt_channels_paula_map`. Any four Paula tracks among channels 1–16 can use those
slots. Continuing tracks retain their assignments through mute/solo and route
edits. A removed track's reader must confirm stop before its slot can be assigned
to another track; pending or failed stops retain the previous map and block new
triggers. Confirmed stops within a partially completed attempt stay stopped.

Each invoked start retains a pinned cache lease, including uncertain or failed
starts. A replacement acquires and validates its candidate before stopping the
old reader, so memory refusal preserves playback. If stopping fails or remains
pending, the unstarted candidate is unpinned and the old data remains owned.
Active voices may finish against retired data after master edits/undo; new
triggers resolve only the current revision. Phase-preserving period/volume
controls retain ownership; uncertain controls require stop or replacement.

The explicit segment request accepts even byte offsets and lengths of 2–131070
bytes (1–65535 words), including an explicitly requested final silent padding
byte. It refuses unsupported alignment, out-of-range segments, zero periods
and volumes above 64 before allocation. Stereo side remains explicit. It does
not infer loops, round fractional phase or encode a zero length register.
The eventual native adapter must validate its clock and device capabilities.

Close blocks new starts, attempts every held reader once and retains all pending
contexts. An optional quiescence callback is required when interrupts or driver
callbacks retain contexts beyond per-slot stop; it must be bound before any
start. Cache/context disposal happens only after all stops and quiescence are
confirmed. The owner supplies no wait loop or forced release.

Injected host regressions exercise ownership and geometry across 8/16/24-bit
masters; the Exec fixture checks Fast master and Chip cache allocation when
executed. On 30 September both routed cache and voice Exec fixtures passed in
the coordinated shared030 emulator, with zero owned Fast bytes on release,
confirmed DMA-off/exact-path cleanup and explicit window release. These fixture
callbacks model readers; they do not perform audible DMA playback.
Native DMA/audio.device/timer
ownership, shared-sequencer dispatch, mixed-backend timing and end-to-end playback
remain unfinished. The classic replay and accepted display layout are unchanged.

## Routed Paula shared-sequence capability gate

`paula_render_voice` translates an exact initial mono one-shot from the audited
renderer into an aligned signed8 cache segment and explicit period/volume plan.
The caller supplies the actual target clock and safe period limits. Frequency
rounds to the nearest period register (ties upward); out-of-range pitches refuse
instead of clamping. This changes no source PCM or sample rate. Gains must match
the selected physical slot's fixed left/right position and an exact volume0–64;
unsupported panning or fractional hardware volume refuses.

`paula_preflight` silently traverses the complete shared16-track sequence,
retaining global tempo, delay and end commands from non-Paula tracks. It checks
Paula trigger/control/stop actions and exact sample-descriptor identity before
any future output, including unsupported notes on late rows. Non-Paula audio
operations remain the other backend's responsibility; this gate does not
qualify them or relax the renderer's MIDI/effect restrictions. A successful mask
identifies only Paula-triggered masters. Failure clears that mask; batch held
state commits only after every action passes.

Two bounded caller allocations cover the plan and sequence; every failure
releases them. Capability traversal does not mix PCM, acquire source/cache
leases or invoke drivers. Inputs remain borrowed and immutable during the call.
Row-range restoration, loop/segment/repeat handoffs, stereo sources,
interpolation and fractional cursor restoration are currently refused. These
need exact plans and independently retained pending-source leases before
support is enabled. The gate is a preparation seam; owned prepared batches,
mixed-backend scheduling and native DMA dispatch remain unfinished.

Five isolated host regressions and selected Amiga fixture builds passed; the
native Exec capability fixture then passed in the coordinated shared030 emulator
on 30 September, with 45 Fast allocations and zero owned bytes on release.
Independent locked DMA-off/exact-path cleanup preceded explicit window release.
This is shared-timeline capability and allocation evidence, not device playback.

## Prepared Paula batches (software seam)

`paula_dispatch` consumes the shared renderer's ordered plan with an explicit
clock/register capability declaration and a revision captured after bridge sync.
It first checks the complete plan, stable track map and current held state. It
then acquires every prospective signed8 playback cache and validates every actual
address before the first stop/start/control callback. A fixed64-entry caller
workspace bounds bookkeeping. Capacity, stale map/revision, foreign descriptor,
geometry or control refusal invokes no device callbacks and preserves old readers;
unchanged master promotions and unpinned cache warming may remain.

Only mono exact one-shot trigger, phase-preserving control and confirmed stop
are supported. Multiple actions retain order; each prospective trigger owns a
separate lease, including shared-cache hits. After master promotion, the complete
plan and all pinned addresses are rechecked. Drivers and allocators must not edit
or reenter. Masters retain their8/16/24-bit values.

An uncertain start/control or pending/failed stop poisons the owner against new
batches, releases only unstarted candidates and attempts each remaining held
slot's stop once during failure cleanup. Unconfirmed leases remain until ordinary
close confirms stops and context quiescence. A partially applied batch must never
be retried. This seam does not own a complete song/master lifetime, schedule mixed
backends, restore fractional cursors, handle repeat sources or program native
Paula DMA. Those remain separate requirements and acceptance gates.

## Owned Paula shared-sequence session

`paula_song` claims an idle injected voice owner and publishes a cancellable
handle. First preparation traverses the complete shared sequence and transfers
that same sequence rewound for playback. Subsequent preparation reserves one
used master or copies at most4096 PCM/marker bytes. No callback output occurs
before every used source is pinned. Unused masters remain unpromoted. Options
and actual-clock capabilities are copied; sample precision/history stay intact.

This initial session supports selected Paula tracks only. The remaining tracks
still contribute global16-track tempo/delay/end flow. Mixed selected outputs
refuse explicitly; a mixed scheduler still needs to coordinate all backends on
one sequence. Row ranges, loops/repeat/segment plans and fractional restoration
remain refused. Initial static validation, full capability traversal and cache
conversion remain synchronous and require finite tick/frame/memory budgets;
there is no hard latency or native timing guarantee.

The exclusive session token blocks public direct sync/trigger/control/stop/close
and dispatch calls. Each advancement checks sampler generation, borrowed project
header, voice/bridge/API/map identities and exact retained master versions.
Channel selection is a permitted UI cursor change. Patterns/orders/sample arrays
must remain immutable; header guards cannot detect arbitrary in-place writes.
The editor binding described below closes this session before edits, undo,
replacement or disposal through the existing mutation barrier. External callers
must also use that barrier before changing borrowed arrays.

Runtime failure discards the sequence, blocks repeated output and requests only
bounded stops. Confirmed per-slot stops and adapter quiescence precede release
of master pins, unpublished promotion jobs and controller storage. Failed close
retains the handle and contexts for retry. DONE also requires close. Preparation
cancellation may leave already-promoted unchanged sampler versions. Successful
close clears the voice owner/cache bridge; restart must rebind them.

Host and native Exec fixtures cover8/16/24-bit precision, selective promotion,
full global flow/measurement parity, exclusive ownership, protocol refusal, late
capability failure before pins, memory refusal, stale settings, uncertain start,
partial preparation cancellation and retained ownership through failed cleanup.
This is software ownership evidence; native PLAY/device/timer/mixed-backend and
physical sound/performance acceptance remain separate.

## Editor ownership barrier for Paula

`editor_paula` attaches the owned shared-sequence session to the existing editor
change barrier. Attachment refuses another installed barrier; begin requires the
same editor sampler and project as the voice bridge. Each edit/import/undo or
external replacement/disposal preparation requests session close first. Pending
reader stops or unconfirmed adapter quiescence veto the mutation while retaining
the session, master pins, cache leases and unpublished preparation jobs. A later
confirmed close permits the ordinary transaction. Failed detach retains the
binding; callers must detach before freeing or reinitializing its editor/context.

The fixture covers8/16/24-bit masters, exact serialized project preservation,
reverse/undo/redo, raw import veto, external replacement/disposal preparation,
conflicting attachment, pending stops, rejected quiescence and cancellation of
an unpublished promotion. Host sanitized tests and selected Amiga builds pass.
The exact native Exec fixture passed on shared030 with81 Fast allocations,
zero owned bytes and actual Chip allocations. Guarded owned-file cleanup and
independent subsequent locked running/all-four-DMA-off checks pass; evidence is
in `evidence/enhanced-editor/editor-paula-ownership`.

This is an injected software ownership binding. It does not wire native PLAY,
DMA, timers, output or UI. Initial capability traversal and cache conversion are
still synchronous; prepare-ahead timing, exact repeat/segment leases and shared
mixed-backend scheduling remain unfinished. Physical hardware was not probed.

## Paula preparation separated from output

`pt_paula_prepare` builds a copied plan/capability batch without device callbacks,
retains its selected Chip cache leases and exclusively claims the voice owner.
Other public trigger/control/stop/close/dispatch calls refuse while it is ready.
`pt_paula_apply` checks the captured project header, map, callback/quiescence
identity, held voice leases, cache revision, exact trigger PCM descriptors and
addresses before output. Channel selection remains a permitted cursor change.
Apply performs no cache acquisition or sample conversion; driver callbacks remain
an injected synchronous contract. Stale or invalid preparation emits no output
and retires unstarted leases. Cancellation preserves current readers. Partial
runtime output still poisons the owner and retains unconfirmed active leases for
ordinary stop/quiescence cleanup. Borrowed sample/event arrays must stay immutable;
arbitrary in-place PCM/event writes remain outside the guarded contract.

`pt_paula_song_stage` (and the editor wrapper) advances and prepares the completed
logical interval's plan before `complete` applies it. Repeated stage is idempotent.
Close/failure cancels candidates before reader cleanup, retaining the ordinary
session/master ownership until stop and quiescence succeed. Existing callers
that omit stage retain the synchronous prepare/apply path. No independent song
engine or four-track projection is introduced.

Preparation still allocates and converts synchronously. This seam does not
provide bounded conversion jobs, a deadline/prepare-ahead clock scheduler or
mixed-backend timing. Preparing all song caches eagerly is intentionally avoided:
only the candidate batch and current readers pin derived copies, leaving other
copies evictable within the existing budget. These remaining scheduling and native
output requirements are distinct from the implemented ownership seam.

Sanitized dispatch/song/editor/voice host fixtures and six selected Amiga C targets
plus guard pass. Native Exec dispatch/song/editor fixtures pass on shared030,
including copied inputs, blocked competing ownership, cancellation/stale refusal,
idempotent staging, allocator refusal during apply and uncertain reader cleanup.
Each run has guarded owned-path cleanup and an independent subsequent locked
running/all-four-DMA-off/absence check. Exact evidence is in
`evidence/enhanced-editor/paula-staged-output`. No physical hardware was probed;
these fixtures establish software ownership, not actual audio/timing acceptance.

## Bounded prepared-master Paula Chip conversion job

`sampler_paula_internal.h` supplies a private job for fully validated, immutable
projects with a genuine held current master pin. Begin checks bounded metadata,
retains its own exact master reference and reserves an unpublished Chip cache
lease without bulk conversion. It uses the existing incremental PCM converter:
each step packs/copies at most256 output bytes, checks revision/header/routes and
exact master identity, and publishes/transfers the lease only on the final chunk.
Cache hits transfer a lease immediately. Cancellation or failure releases only
the job's unpublished lease/master reference; already-active readers retain
their own cache leases. Partial data cannot authorize a playback location.

This private path avoids redundant full PCM validation between steps. Callers
must validate before preparation and retain immutable arrays/contexts through
completion/cancel. Allocation/eviction and metadata work are synchronous; no hard
latency guarantee is claimed. Public arbitrary-source acquisitions retain their
full validating behavior. The staged dispatcher/song still uses synchronous
acquisition; wiring this job into that owner is the next integration requirement.

Tests cover8/16/24-bit masters, five steps for an odd1025-frame copy with exact
padding, no allocation during steps, retained source lifetime after the caller
unpins, cache hit/busy behavior, cancellation, descriptor/generation invalidation,
budget pressure preserving an old reader lease and pending close cleanup. Five
sanitized host modules and selected cache portable/native Exec builds plus guard
pass. The exact native Exec cache fixture passes shared030 with zero owned
allocations; guarded cleanup and independent subsequent locked running/DMA-off/
owned-path absence checks pass. Evidence is in
`evidence/enhanced-editor/paula-chip-jobs`. No native audio or physical testing.
