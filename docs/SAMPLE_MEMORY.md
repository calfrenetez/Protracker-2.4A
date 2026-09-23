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

The current classic Paula bridge still allocates a whole private Chip-RAM MOD
snapshot per playback session and frees it after stopping replay. It protects
masters from EFx mutation, but is **not yet a selective per-sample cache**.
AmiGUS cache eviction and Studio streaming are not completed by the master
allocator change. Native render/stem helper allocations and remaining generic
utility buffers also need their own allocation audit.

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
