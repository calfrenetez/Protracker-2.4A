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
