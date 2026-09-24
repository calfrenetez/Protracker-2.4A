# Paula Chip reserve admission

The native Paula cache allocator now queries free Chip RAM on every new
allocation and admits only allocations leaving 512 KiB for display/system use.
The initial cache budget also excludes this reserve. The silent DMA word uses
the same check; restart may release retained idle caches before retrying it.
Master data and pinned DMA buffers are not eviction candidates.

Host ASan/UBSan checks cover exact reserve boundaries, zero/overflow rejection,
changed free memory after cache initialization, allocation failure/fragmentation,
unpinned eviction, pinned content preservation and complete release. Existing
Paula preflight, selective cache and sample preview checks also pass.

This is a conservative admission check, not an atomic system-wide reservation.
Other applications can still consume memory concurrently. Physical memory,
audio and timing acceptance remain separate. No AmiGUS hardware operations.

Pinned full native build passed. Shared030 run1790240003728563000 passed all
five native regression executables (rc0), including repeated partial Chip
allocation failure cleanup/restart, cache reuse, immutable masters, audition,
live edit and busy-channel refusal. All four DMA channels were off before exact
owned cleanup and explicit release to AmiConnect. CIAA fallback remains NOT RUN
because Workbench owns timer B. Result JSON records exact executed hashes.
The build includes unrelated display edits; no editor UI qualification or release
package update is implied. No physical testing took place.
