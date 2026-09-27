# AmiGUS voice command preparation — 27 September 2026

Software-only preparation from validated project metadata and a pinned playback
cache. No register emission or real start/stop callbacks. Source observations
record pinned workbook cell ranges/hashes and the utility's rate formula.

- 11 related AmiGUS ASan/UBSan fixtures: PASS (5.692s).
- Final three wavetable fixtures including command integration: PASS (8.238s).
- Native cross-build: all53 dependency hashes and binary verified.
- Shared030 run `render-files-1790475765845129000`, coordinated with AmiConnect
  and Scott, shared lock/live identity guards and90-second deadline: RC0.
- 40 Fast allocations, zero final owned bytes; budget refusal without Chip fallback.
- Completion, all4DMAoff, exact cleanup and explicit release verified.
- No timeout/retry/reset, physical operations or remaining resource hold.

Binary SHA256: 643249eebb7f8d1ee2873fb949f519b2ad78aba8468f3ff4afc1d5ca28068a2c
Binary bytes: 139560. New native editor output is not enabled.

Tests cover selected-channel byte lengths,24-bit master preservation, forward
loop bounds, odd8-bit offsets/loop/end refusal,25-bit boundaries, rational rates,
volume/pan extremes, unsupported modes and unchanged output after refusal.
Invalid commands are rejected before any old-voice stop or sample upload.
Arena init/attach now reject ranges that exceed the25-bit address port.

The first standalone test failed on a manually entered expected pitch constant;
an independent Python Fraction calculation corrected it to46345033. Production
rate arithmetic did not change. All retained results use the corrected oracle.

Physical capacity, endpoint convention and stop-read completion semantics remain
unverified. Plans carry half-open software bounds; hardware lowering is deferred.
Envelope control remains disabled because the maps and utility disagree on its
bit position. These are documented protocol limits, not a request for approval.
