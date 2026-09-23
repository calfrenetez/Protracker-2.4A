# Preview conversion cancellation — 2026-09-23

Full pinned native build PASS. Coordinated shared030 Paula/cache/PCM regression
PASS. Injected native cancellation was observed once, left idle replay stopped,
and on an already playing instance retained its data pointer and cache generation.
No replacement playback started. Full precision/rate/loop/stereo regressions also
passed. All3 return codes0, all4 DMA channels off, unique files/script removed,
lock released and AmiConnect explicitly notified. Hashes/logs retained here.

Host ASan/UBSan tests inject cancellation before writes, during stereo filtering,
at same-rate completion and then retry successfully; master bytes unchanged.
Partial staging is discarded by the native caller. Same-rate precision conversion,
validation scans and stereo split remain synchronous; no latency bound claimed.

Editor audition is wired to its existing modal progress/Escape callback. Full
interactive Escape/refresh verification has NOT been performed this milestone.
Full editor builds include unrelated dirty display work; no clean package claim.
No physical/analogue/AmiGUS testing. Shared baseline unchanged. CIAA fallback
execution skipped when Workbench owns its vector.

Full host suite:

```
Ran 105 tests in 175.043s

OK
```
