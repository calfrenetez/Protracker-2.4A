# Native sample-memory suite — 2026-09-23

Added PTPlaybackUploadTest and PTPreviewPCMTest to the pinned native build and
existing guarded shared030 runner. Full native build PASS. Three focused host
ASan/UBSan tests PASS (PCM representations, upload ownership/chunks, preview).
Production playback sources were unchanged; the prior full105-test result is
recorded in cancel-paula-preview, not claimed as a new full-suite run here.

All five native binaries returned0 and their required assertions passed:
- PTPlaybackUploadTest: opaque device descriptors, complete-upload publication,
  failed partial transfer cleanup/retry, pinned resources, chunk offsets, widths,
  endian/channel/pad combinations, tiny-buffer canaries and master preservation.
- PTPreviewPCMTest: precision/filtering, stereo, loop mapping and cancellation.
- PTPlaybackPCMTest, PTSampleCacheTest and PTPaulaTest: existing regressions.

Device memory here is a test double compiled for68030. This does NOT establish
an AmiGUS allocator, transfer protocol, card upload or live voice implementation.
No physical hardware accessed. CIAA fallback remains skipped when occupied.

All4 DMA channels off afterward; unique script/files cleaned under the shared
lock and explicit release sent to AmiConnect. Baseline/startup unchanged. Binary
hashes and five logs retained alongside this report. Full editor build includes
unrelated dirty display changes; no clean release package claimed.
