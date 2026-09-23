# Forward-loop derived Paula preview — 2026-09-23

Pinned native build PASS. Shared030 emulator regression PASS: a4000Hz true24
five-frame master with loop1..5 produced a12-byte classic-rate copy with loop2..10
(header start1 word, repeat4 words), while master endpoints/rate/data remained
unchanged. Existing preview, cache and replay checks also passed. All3 return
codes0, all4 DMA channels off, unique files cleaned and explicit release sent.
Hashes and logs are alongside this file. No restart, startup edit or physical
access. CIAA fallback remains skipped when Workbench owns its vector.

Host test covers scaling, nearest-word ties, end clamping to real audio rather
than padding, output-preserving refusal of collapsed/one-word loops, invalid
ranges and aliased output coordinates. Generic PCM buffer alias tests still run.
This is addressing/ownership evidence, not seamless-loop listening or physical
performance acceptance. Stereo and ping-pong/crossfade preview remain unsupported.
Unrelated display work remains uncommitted; no clean full-editor package claimed.

Full host suite:

```
Ran 105 tests in 210.423s

OK
```
