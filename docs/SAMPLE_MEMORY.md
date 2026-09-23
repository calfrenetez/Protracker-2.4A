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

The bridge still requires a classic-compatible four-channel project; implicit
conversion of enhanced samples has not been added. AmiGUS cache eviction and
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
