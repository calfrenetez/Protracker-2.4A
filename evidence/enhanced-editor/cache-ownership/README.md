# Versioned cache ownership and Paula restart reuse — 2026-09-23

102 host tests passed in 168.959 seconds, including ASan/UBSan ownership tests.
The complete pinned native cross-build passed.

The portable cache core and native Paula integration were cross-built and ran on
the coordinated shared 68030 emulator. Exact hashes are in result.json.
PTSampleCacheTest verified unpublished staging, pinned retired revisions, fresh
versions, same-size refill, LRU eviction, backend failures, stale handles and
complete release. PTPaulaTest verified reuse of the same Chip allocation during
Play restart while reloading EF-mutated/master-edited bytes, and full release on
explicit Stop. Its prior placement, pressure, replay and ownership checks passed.
CIAA fallback execution remained skipped because Workbench owns that vector.

Both executables returned 0; all four DMA channels were off; unique run files
were removed and the lock/resource released. Shared guest left running unchanged.
No physical or analogue acceptance. The broader checkout contains uncommitted
display work; neither native test links that work. No clean full-editor release
package is claimed.

AmiGUS can reuse this resource-ownership core, but its card allocator/upload
adapter and physical validation remain unimplemented. Paula refreshes each
restart rather than treating EF-modified playback bytes as a pristine cache.
