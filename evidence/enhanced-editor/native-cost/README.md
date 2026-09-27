# Native service cost diagnostic — 27 September 2026

Shared030 run `render-files-1790506832936264000` passed, RC0, using the staged
candidate (404492 bytes, SHA256 b8fb5f9893cd7865b9b6d83c9965b7f3375184c91b2c58def8d8303e4b97dac1).
All171 input hashes, generated font and binary were verified against the index.
Native cross-build and editor-main syntax passed. Production source is unchanged.

The fixture uses real EClock duration readings around an INJECTED logical song
clock and FAKE voice callbacks. One tiny looped24-bit master supplies one or16
voices; ready calls repeat four times. Production source/version/address checks
remain enabled. Startup asserts no allocation or upload, and close releases all
cache/master leases.1211 tracked Fast allocations, zero final ownership, no Chip
fallback. Timers/watchdogs closed, all four DMA channels off; exact owned run and
launcher absent after cleanup. Shared030/DevBench explicitly released to AmiConnect.

At709379Hz, ready calls took1610..2290ticks (about2.27..3.23ms). One-voice startup
was10737ticks (15.14ms):663 before the logical reader,6798 from reader to first
callback,0 callback spread,3276 from last callback to return.16-voice startup was
76025ticks (107.17ms):661 before reader,23789 to first callback,35198 first-to-last,
16377 to return. These costs include instrumentation/EClock reads and emulator
scheduling; they are observations, not isolated CPU cycles or physical benchmarks.

The exact-frame service timestamp does not make subsequent synchronous callbacks
instantaneous. Neither this diagnostic nor a prepared cache proves a realtime
commit budget. No hardware output, late-tolerance policy, ownership shortcuts or
native PLAY enable were introduced. Precomputing voice commands before deadlines
is separate follow-up work; native bus/cadence qualification remains outstanding.
