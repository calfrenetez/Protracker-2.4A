# Native consumer and full output-chain ownership

2026-09-24. Eight Studio host sanitizer tests pass (11.162s), pinned native build
passes. Shared030 PTExecStudioConsumerTest and PTExecQueuedSongTest each rc0,
with 6 and25 tracked Fast/not-Chip allocations respectively, zero final owned
bytes and budget refusal without Chip fallback.

Real song output passes through pump, queue and consumer to a delayed fake
transport. Exact reference audio survives busy submits and delayed completion;
normal, lead-in and pre-roll use256-frame native blocks (host1/17/256). Separate
17-frame stopped-song case releases pins but retains copied output through failed
cancel until confirmation. Consumer fixture also covers polling failures.

Run render-files-1790221074542748000 used fresh AmiConnect clearance, shared
identity/health guards and test.lock. Completed-run cleanup and explicit release
are complete. Binary hashes in result.json. Initial host fixture pre-roll bound
was corrected before this qualified build/run.

No physical operations, actual audio transport, DMA or deadline qualification.
Tracked allocator evidence excludes static fixture arrays, stack and CRT.
Native build includes unrelated display work and does not qualify UI/release.
