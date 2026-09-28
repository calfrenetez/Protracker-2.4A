# Wavetable callback lifetime checks

Final cumulative shared030 run: `render-files-1790558636012127000`.
`PTExecEditorWavetableTest`: 420216 bytes, SHA256
`5ed539aedf12e108ed927df67e1c53478037389b0014045eec5a3d9875891bdd`.
179 build inputs matched the staged candidate (generated font checked separately).

Host ASan/UBSan fixtures passed. Native cumulative fixture returned 0 with
1265 Fast allocations and zero owned bytes. It covers pending/error/unknown
quiescence replies, held interrupt guards, cancelled preparation, failed preflight
with a published owner, and editor mutation/disposal vetoes until confirmed idle.
Synchronous open refuses asynchronous cleanup adapters before doing work.

The voice bus is injected. Existing real timer.device diagnostics also ran;
these do not qualify playback deadlines or any physical hardware. No AmiGUS MMIO,
audio output, environment changes, clock setting or physical operation occurred.
Runner cleanup and an independent fresh-lock check confirmed the running guest,
all Paula DMA channels off, and exact run/launcher absence. Explicit RELEASE was
sent to AmiConnect. The physical Amiga remains off and unprobed.

An earlier cumulative run (render-files-1790558361103646000, 1262 allocations)
passed, but was superseded after review added the failed-preflight regression.
Only the final candidate above is qualified here.
