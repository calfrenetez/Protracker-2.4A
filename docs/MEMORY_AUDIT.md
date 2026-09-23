# Native memory audit — 2026-09-23

This is an implementation/status inventory, not physical-hardware acceptance.
The accepted layout and unrelated prepared-display work remain unchanged.

| Path | Current policy | Remaining boundary |
| --- | --- | --- |
| Enhanced document, sampler versions/undo, song storage | Shared queried Fast-first budget; no Chip spill on Fast exhaustion | Physical pressure/fragmentation and performance |
| Paula playback | Selective derived Chip copies, pinned during replay; master retained | Physical audio/listening; enhanced mixed16 dispatch |
| WAV/stem render and bounce | Bounded allocator-owned mixer, file and batch buffers; direct master PCM | Live Studio is not implemented by offline rendering |
| Project/sample/MOD file save | Bounded native file workspace and4096-byte descriptor verification; explicit MEMORY result | Runtime descriptor/stdio diagnostics internals not instrumented |
| Project/sample import | Bounded master-allocated payload,64MiB ceiling, descriptor reads with short-read/EINTR handling and exact EOF; decoding staged transactionally | Runtime descriptor internals not instrumented; same-size concurrent edits are not detected |
| Recent-files persistence | Separate calloc workspace (two lists, encoded record, paths), stdio read/write | Not yet connected to master pool; no sample payload stored here |
| Display | Fast-preferred canvas with public fallback; display rasters and custom-chip requirements | Separate display budget; unrelated dirty prepared-display changes unqualified |
| AmiGUS cache seam | Versioned, pinned, evictable opaque resource cache and bounded upload conversion | Pinned SDK lacks actual sample allocation/upload interface |

Exec-backed emulator fixtures verify explicit allocator pointers with TypeOfMem
and require pool-owned bytes return to zero. They do not account for allocations
inside libnix, DOS, graphics, file requesters or diagnostics. Runtime archives are
pinned/hashed by the build; hashes do not prove their allocation policy. Remaining
small locals and native UI frames are stack-based; per-function compiler estimates
are not whole-stack peaks. Existing master/sample data must not be downgraded to
satisfy these auxiliary paths.

Next actionable gap is recent-file workspace ownership and descriptor I/O.
Physical A1200 tests require fresh AmiConnect ownership/recovery-hold review.
No actual AmiGUS or live24-bit streaming acceptance is claimed.
