# Optional routed Paula sample cache owner

New standalone sampler/cache bridge creates selected signed8 word-padded playback copies for Paula-routed tracks among all16 channels. No allocation at bind; explicit stereo side, immutable source pin during conversion, revision/table/route invalidation, pressure eviction of unpinned copies, and busy-close retention of leased data. No master precision or sample-rate changes.

Six isolated sanitized host regressions passed in11.399s: new owner, generic cache/conversion, channels, existing wavetable bridge and existing Paula replay preflight. The owner tests8/16/24-bit input, track16, fifth-route refusal, private padding, explicit stereo side, Fast/cache allocation refusal, reuse, eviction, edits/undo/routes, stale-location refusal, active leases, close and version exhaustion.

Both native fixtures and the guard built successfully. 114 indexed source hashes and 3 binary hashes matched the manifest. Initial compilation rejected misleading indentation in the test allocator; corrected snapshot and original diagnostic are retained. The Exec fixture checks Fast masters and Chip cache storage when run, but has NOT executed.

This is the cache ownership layer for future routed Paula playback. It does not allocate physical voices, program DMA, schedule shared song time or enable mixed-backend/native PLAY. Caller must confirm all readers stopped before unpinning. Existing four-channel replay/refusals and accepted display layout are unchanged.

No emulator or physical operations this turn. Previous shared030 identity refusal remains unresolved. Full149-target integration and emulator execution remain pending; the prior147-target/207-host baseline belongs to de123db.
