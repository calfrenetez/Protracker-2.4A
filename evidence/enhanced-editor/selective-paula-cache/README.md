# Selective Paula cache — 2026-09-23

101 host tests passed (170.298 s), including sanitized cache packing/invalidation
and allocator-pressure tests. Complete pinned native cross-build passed.

The coordinated shared 68030 emulator ran PTPaulaTest, hash in result.json.
Unused sample data was excluded from Chip RAM; Chip/Fast allocation classes,
new-instrument and changed-master stop/rebuild assertions passed. Existing
playback/live-edit/ownership/empty-guard tests also passed. CIAA fallback execution
was not run because Workbench owned that vector; the log identifies this limit.
All four audio DMA channels were off at completion. Run-owned guest files were
removed and the resource explicitly released to AmiConnect and infrastructure
owners. No physical hardware or analogue audio acceptance is claimed.

The shared checkout contains unrelated uncommitted display work. This native
Paula test does not link that editor/display work; the full editor build does.
No clean full-editor release package is claimed. Reproduction:
reserve the shared guest with its owners, build using the pinned compiler, then
run `tools/shared_infra_paula.py` with the shared harness Python runtime. It takes
the nonblocking shared lock and verifies process/profile, CPU and bridge target.

Remaining: per-sample allocation/eviction, Fast pattern storage, enhanced Paula
conversions, AmiGUS caches and Studio streaming. The current cache is one pinned
session snapshot and scans all stored patterns conservatively.
