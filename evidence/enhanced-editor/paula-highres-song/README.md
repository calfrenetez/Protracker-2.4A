# Precision-preserving Paula song playback

Candidate source identity: all151 source hashes in build.json matched the staged
source; all7 build artifacts were verified. Product editor:262564 bytes, SHA256
`83353fbdf697635278e53bbbcaedc5f76c45a984663b496c172df567daa5df2d`.
Native Paula fixture:120360 bytes, SHA256
`9e40d6f0b0b9accd64b4aff2d18d50e77616dbbee54d31a1cdcc607a59a93a70`.

Six focused host sanitizer tests passed, plus editor guard and sampler tests.
The first supplementary editor-host invocation had two loader errors (wrong module
name and missing tests search path); corrected invocation passed, both logs kept.

Native run `paula-cache-1790559230789129000` passed all5 fixtures. It exercises
16/24-bit master-preserving rounded8-bit Chip song copies, selective allocation,
unchanged source save, cache generations/refill, busy refusal, effects, partial
allocation failure and stop. CIAA fallback execution was NOT RUN because Workbench
owns timerB; that vector was preserved. No physical audio/listening acceptance.

Final editor UI run `paula-highres-ui-1790559767477699000` passed song and pattern
playback, an edit exclusively in24-bit low bits stopping DMA despite identical
8-bit quantization, exact undo, restart, save, reopen/play and second exact save.
The original, saved and reopened project bytes match. Fresh native guest bitmaps
were inspected; accepted classic layout remains intact. Both processes exited
normally with RC0, allfive isolated ENV settings were exactly restored and all
DMA channels stopped. Independent lock/identity/running/absence checks preceded
explicit RELEASE. The first UI run's premature Stop assertion is retained under
ui-first-run; it was separately cleaned/released and is not counted as a pass.

This implements precision-only cache conversion for otherwiseclassic-compatible
four-channel Paula projects. It does not add nonclassic sample rates/stereo/slices,
multichannel routing, AmiGUS/Studio output or physical-machine qualification.
