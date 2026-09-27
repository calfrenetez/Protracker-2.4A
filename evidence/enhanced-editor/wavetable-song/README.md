# Owned wavetable song lifecycle — 27 September 2026

`wavetable_song` gates sequence use on complete capability analysis, pins only
actually triggered masters and dispatches at audited interval boundaries.
The injected voice owner is exclusive until confirmed close. Options/format
are copied; project arrays remain borrowed/immutable and native editor hooks
are still required before user-facing playback can be enabled.

Final host ASan/UBSan fixture PASS5.940s. Native cross-build75 dependencies and
binary verified. Shared030 run `render-files-1790477656284461000`: RC0 within90s,
115 Fast allocations, zero final owned bytes, no Chip fallback. Fresh separate
AmiConnect/Scott coordination, shared lock/live identity checks, all4DMAoff,
exact run/launch cleanup and explicit release verified. No remaining hold.
Binary184360 bytes, SHA256
`9524b512571e8b2c47c9c8fe884e050938c6c4d0d68c75851f923fb233e52cc4`.

Tests include late-stereo refusal before pin/upload/start, row-range refusal,
failure at each of five successive allocation stages, selective source pins,
second-owner refusal, protocol errors without advancement, zero-frame lead-in,
full frame totals against renderer measurement, confirmed natural end, early
close and pending natural-end stops. Caller option/format changes do not affect
a running song. Stale sampler generation blocks further dispatch; an uncertain
start retains its device lease and master pin through pending stop. Releasing
the sampler's current ownership still leaves the session's source alive until
confirmed close. Exact enhanced saves preserve true24-bit data; no allocation
remains after final teardown. Existing preflight/voice/cache fixtures also pass.

One early host fixture incorrectly assumed the first omitted lead-in span must
trigger immediately; speed2 produces multiple zero-frame spans. The fixture now
uses the audited interval protocol until the note is actually fetched. Final
host/native checks passed; no emulator retry/reset was needed.

No device MMIO, real library/card output, audible acceptance or physical test.
Consume records externally elapsed frames; it is not a clock/scheduler. Row-range
pre-roll, native editor stop-before-edit guards, real endpoint/stop semantics,
card capacity and performance remain separate work. An unresolved stop must
retain the controller handle and all master pins until a later confirmed close.
