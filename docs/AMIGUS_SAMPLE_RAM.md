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
reacquiring the reservation does not validate old card RAM. The existing native
reservation adapter is PCM-only and is not a wavetable ownership implementation.

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

Remaining: verify card/firmware capacity and Mini bus transactions, implement a
wavetable reservation owner and native binding, and connect voices with confirmed
stop/transfer completion before releasing leases. The injected bus establishes
software behavior only. There is no real card access, device upload, audible
playback, physical performance or physical acceptance from these fixtures.

The native fixture also passes shared030 in its coordinated 90-second window
(run `render-files-1790473124808998000`): RC0, one Fast metadata allocation, zero
final owned bytes and budget refusal without Chip fallback. All four DMA channels
were off before exact owned cleanup and explicit resource release. No timeout,
retry or reset was needed. This still uses the injected bus, never actual RAM
uploads to an AmiGUS.
