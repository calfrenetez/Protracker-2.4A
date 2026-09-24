# Sample memory and playback ownership

The enhanced project owns the authoritative sample, including its declared
8/16/24-bit precision, loops and slices. PCM is represented by signed `int32_t`
elements in the declared precision; 24-bit samples retain their low eight bits.
Playback representations must never become the project's source of truth.

## Implemented native master allocation

`src/native/master_memory.h` supplies the native editor's document, donor
sample documents, sampler versions and undo, song storage, editor/history
structure, and whole-file import/export buffers. They share one allocation
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
