# Shared030 queued-song and pump memory qualification

Fresh AmiConnect confirmation, shared test.lock and live target guards. Both
PTExecQueuedSongTest and PTExecStudioPumpTest passed rc0 with behavior and memory
markers.21 and4 tracked Fast/not-Chip allocations respectively; final owned bytes
zero and production budget refusal preserved. Queued guest song uses256-frame
blocks; host retains1/17/256. Pump covers stalls and failure with held consumer lease.

Run render-files-1790219072182937000 completed; exact run/launcher cleanup verified,
lock and window explicitly released. Binary hashes in result.json. Host fixtures
and pinned native cross-build passed. No UI/physical/device-audio operation.

Tracked allocator and algorithm proof only, not CRT/static memory, DMA ownership,
physical deadlines or AmiGUS transport. Native build includes unrelated display
work and is not an editor release/layout qualification.
