# Wavetable voice lease validation — 27 September 2026

Production software voice owner around the sampler revision/cache bridge.
Callbacks remain injected: no real card, library, voice register or audio I/O.

- Related three wavetable ASan/UBSan fixtures: PASS (8.157s).
- Final voice fixture with additional partial-upload failure: PASS (3.901s).
- Native cross-build: 50 dependency hashes and binary verified.
- Shared030 run `render-files-1790475253667152000`, separately coordinated with
  AmiConnect/Scott, shared lock/live guards and90-second deadline: RC0.
- 40 Fast allocations, zero final owned bytes, budget refusal without Chip fallback.
- Completion, all4DMAoff, exact owned cleanup and explicit release verified.
- No timeout, retry, reset, hardware operations or remaining resource hold.

Binary SHA256: b5ec7d98f94b2373ea1c01509a02689aa866b3916c5990ac6b0287f53c4d62df
Binary length: 133612 bytes. Build manifest records all50 input hashes.

Covers16 leases sharing a representation, retrigger, memory/upload refusal
without stopping old playback, master edit/undo, unstarted candidate release,
uncertain starts, pending/failed stops, lost ownership and bounded close.
Exact enhanced saves preserve24-bit source. Hardware stop confirmation is a
callback contract, not a measured real-device result. The native editor does
not yet instantiate these adapters; the application package is unchanged.
