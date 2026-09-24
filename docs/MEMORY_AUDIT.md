# Native memory audit — 2026-09-24

This is an implementation/status inventory, not physical-hardware acceptance.
The accepted layout and unrelated prepared-display work remain unchanged.

| Path | Current policy | Remaining boundary |
| --- | --- | --- |
| Enhanced document, sampler versions/undo, song storage | Shared queried Fast-first budget; no Chip spill on Fast exhaustion | Physical pressure/fragmentation and performance |
| Paula playback | Selective derived Chip copies, pinned during replay; master retained | Physical audio/listening; enhanced mixed16 dispatch |
| WAV/stem render and bounce | Bounded allocator-owned mixer, file and batch buffers; direct master PCM | Live Studio is not implemented by offline rendering |
| Project/sample/MOD file save | Bounded native file workspace and4096-byte descriptor verification; explicit MEMORY result | Runtime descriptor/stdio diagnostics internals not instrumented |
| Project/sample import | Positional RAW/WAV/IFF/MOD/enhanced/PP20 readers with fixed caches;64MiB input ceiling and transactional master staging; PP20 retains budgeted unpacked scratch | Runtime descriptor internals and stack placement not instrumented; stable source required |
| Standalone renderer/converter | Native queried Fast-first shared allocator; bounded supported-format loaders; allocator-owned WAV/stem workspaces and no-replace publication | Physical pressure/performance; runtime library internals |
| Recent-files persistence | Native editor supplies master allocator for one cleared workspace; descriptor I/O retries EINTR/short transfers | Runtime descriptor internals not instrumented; legacy API retains heap allocation |
| Display | Fast-preferred canvas with public fallback; display rasters and custom-chip requirements | Separate display budget; unrelated dirty prepared-display changes unqualified |
| AmiGUS cache seam | Versioned, pinned, evictable opaque resource cache and bounded upload conversion | Pinned SDK lacks actual sample allocation/upload interface |

Exec-backed emulator fixtures verify explicit allocator pointers with TypeOfMem
and require pool-owned bytes return to zero. They do not account for allocations
inside libnix, DOS, graphics, file requesters or diagnostics. Runtime archives are
pinned/hashed by the build; hashes do not prove their allocation policy. Remaining
small locals and native UI frames are stack-based; per-function compiler estimates
are not whole-stack peaks. Existing master/sample data must not be downgraded to
satisfy these auxiliary paths.

Explicit import/recent-file workspaces are now budgeted. Runtime library internals,
display allocations and live playback integration remain separate boundaries.
Physical A1200 tests require fresh AmiConnect ownership/recovery-hold review.
No actual AmiGUS or live24-bit streaming acceptance is claimed.
