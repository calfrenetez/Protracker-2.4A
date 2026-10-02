# Incremental mixed preparation qualification — 3 October 2026

Implementation commit `c42d69b` advances one analysis phase per preparation call,
keeps partial source masks private, and cancels unfinished analysis before source
promotion. The same audited sequence transfers once. Initial static/PCM validation
remains synchronous; the operation bounds are not timing guarantees.

Five scoped host sanitizer suites pass: mixed preflight, 255 mixed owner cases,
15 editor lifetime cases, native Paula transport and Studio pump. The staged
indexed editor fixture also passes, excluding unrelated dirty display work.
Three pinned native cross-builds pass; compiler/runtime/transitive source and
exact binary hashes are in build.json. Every build input was verified against
committed tree `c42d69b` before Guest creation.

Exact shared030 qualification `mixed-incremental-qualification-1790983137885073000`
passes all three fixtures RC0:

- Capability parity, bounded steps, hidden masks, transfer, cancellation and late
  refusal: 96 Fast allocations, zero owned bytes.
- Mixed owner: 75 native scenarios including 15 new incremental cancellation,
  stale-source and allocation-failure cases; 795 Fast allocations, zero owned bytes.
- Editor binding: 27 native scenarios (15 lifetime plus 12 private timer/advance);
  243 Fast allocations, zero owned bytes.

Each exact run/launcher cleanup passed, followed by a separately acquired lock,
original sole PID19081/profile/localhost/running68030/all-four-DMA-off check and
10-second exact-path absence observation. Window explicitly released through
AmiConnect; no retained hold/reservation. No reset, resume, restart, retarget,
physical card access or browser control.

Voices/registers are injected; private native alarm lifetime is tested separately.
These passes do not qualify timing, hardware RAM capacity/upload ordering or
completion, real voice-stop, playback, listening or the full editor frontend.
