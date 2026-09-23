# Filtered mono Paula preview — 2026-09-23

Full pinned native build PASS. Shared guarded030 regression PASS, all3 return
codes0, audio DMA off, unique files cleaned and resource explicitly released.
No shared startup/config change or physical access. Native assertion played a
five-frame24-bit master at twice the classic rate through a four-byte filtered
Chip copy. Master bytes/rate/frame count remained unchanged. Same-rate precision
preview and existing playback/cache regression also passed.

See result.json for tested binary hashes, native-paula.log for assertions.
CIAA fallback execution remains skipped when Workbench owns its vector.
This is emulator evidence, not physical timing, analogue listening or AmiGUS proof.
Full editor builds still include unrelated dirty display changes; no clean editor
release package is claimed. Upload-chunk fake-device test execution on emulator
is still pending (this runner executes PCM/cache cores and native Paula).

Changed-rate loops and stereo remain unsupported. Conversion uses the existing
integer antialias filter, bounded Fast-first int32 workspace and derived8-bit
rounding/padding. Original project samples are not changed or saved from caches.

Host ASan/UBSan and full suite:

```
Ran 105 tests in 173.888s

OK
```
