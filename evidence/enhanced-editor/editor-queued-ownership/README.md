# Editor queued Studio ownership

2026-09-24. Host editor tests: 3 passed; Studio tests: 7 passed. Pinned
Amiga build passed. Shared030 PTExecEditorStudioTest passed rc0 with 42 tracked
Fast/not-Chip allocations, zero final owned bytes and budget refusal.

Fixture exercises queued edit, undo, Stop and disposal with a consumer lease;
waiting/pending audio is discarded while leased memory survives until release.
Natural completion drains instead of aborting. Owner retains the queue borrow
until explicit Stop/detach, even after completion.

Run render-files-1790219859363090000 used fresh AmiConnect clearance, shared
identity/health guards and test.lock. Completed run files cleaned exactly and
window explicitly released. Binary hash is in result.json.

This qualifies algorithm and tracked allocator behavior, not device transport,
DMA, real-time performance, physical A1200 or AmiGUS audio. Native build includes
unrelated display edits and does not qualify a UI/release candidate.
