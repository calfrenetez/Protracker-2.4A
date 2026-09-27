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
refuses an empty, unaligned or wrapping region without I/O. A fixed 32-descriptor
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

Remaining: verify card/firmware capacity and Mini bus transactions, implement the
native bus binding, and connect voices with confirmed
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
