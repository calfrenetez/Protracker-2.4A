# Sampler undo-record budget and retained recording ownership

Sample-edit undo records previously allocated memory without charging it to the sampler ceiling. The native master pool still bounded those allocations, but the narrower sampler budget understated actual ownership. The fix checks capacity before allocating a record and charges/releases it on success, rollback, undo-history eviction and redo truncation.

The new allocator-observing test fails the old implementation at `s.bytes == actual used`. After the fix it checks actual payload accounting, a one-byte-short limit, unchanged master/history on refusal and release through eviction/truncation. Seven targeted host tests pass (23.085 seconds).

A separate capture regression passes (3.039 seconds): removing an appended recording through undo, reusing its slot with a new 8/16/24-bit recording and discarding redo preserves the old playback pin. Stale generations refuse new pins; command eviction and sampler release retain the current pin until final unpin. This verifies software ownership, not native device voice behavior.

Full host suite: 207 cases across 172 modules passed in 250.677 seconds. All 147 Amiga targets built; 350 indexed source hashes, one generated source and 147 binary hashes were verified. Logs and manifests are stored alongside this file. The HEAD-dependent editor-reference test requires a separate post-commit run; the other Git helper uses the staged index.

No emulator or physical operations were attempted for this candidate. The prior shared030 identity refusal remains unresolved; runtime acceptance is pending. Unrelated display changes are excluded from the source snapshot.
