# Resolved wavetable dispatch — 27 September 2026

Audited renderer command plans now reach the software wavetable lease owner via
injected start/control/stop callbacks. This does not enable native editor output.

- Two related host sanitizer fixtures PASS in9.830s.
- Final dispatch fixture with additional geometry refusals PASS in5.749s.
- Native cross-build:72 input hashes and binary verified.
- Shared030 `render-files-1790476398105279000`, coordinated AmiConnect/Scott window,
  shared lock/live guards,90-second deadline: RC0.
-50 Fast allocations, zero final owned bytes, budget refusal without Chip fallback.
- Completion/all4DMAoff/exact cleanup and explicit release verified.
- No timeout/retry/reset, physical/device operations or remaining hold.

Binary: 170680 bytes
SHA256: 2831e5b786e6a8127636eec11415145b1eed3bd56b8b68629712a7427633718e

Tests generate actual16-channel render command plans and check initial notes,
control-only pitch slides, independent side gains, explicit Stop/retrigger,
late unsupported-operation refusal before any start, foreign descriptor refusal
without dereference, stale revision refusal and invalid gain limits. Failed
control and a partially applied start batch retain unconfirmed leases and block
further dispatch until cleanup. Exact enhanced saves retain24-bit source bytes.
Direct preparation checks selected ranges, fractional Q32 rates and refusal of
stereo, segment, pending repeat, pingpong and non-initial trigger phase.

This covers ordinary mono trigger/control/stop only. It does not implement
stereo expansion, delayed segment/repeat handoff, private EFx, scheduling, native
register writes, confirmed physical stop, audible output or device performance.
The established Studio implementation remains separate and unchanged.
