# One-to-three-track Paula playback

All151 source hashes matched the staged candidate;7 native build artifacts were
verified. Editor263252 bytes SHA256
`166c72a730e17a9dbb73e857164893dbb6487dd84cefeaf2c0852e3133965628`.
Paula fixture123400 bytes SHA256
`65c27a327078d19b44e2afe235de802f807a748baba3eea1aa93529e9193e2f3`.

Three host sanitizer test groups pass: private padding/unused endpoint isolation,
source immutability, bounded sync and channel routing. Cumulative native run
`paula-cache-1790560276746325000` returned0 from allfive fixtures: actual1/2/3-track
playback, silent unused voices, bounded Fast-RAM padding, future-row edits,
count-change stop and exact source save, alongside prior high-resolution/cache
pressure/effects/ownership checks. CIAA fallback execution remains NOT RUN while
Workbench owns timerB; its existing vector is preserved.

Editor UI run `paula-highres-ui-1790560395004542000` passes a three-track16/24-bit
project: three active DMA voices with fourth off, low-bit-only sample edit stops
DMA, exact undo, pattern restart, save, reopen/play and exact second save. Fresh
guest bitmap inspected: classic layout, empty fourth column. Bothnormal exits
returned0. Allfive temporary ENV settings restored exactly; independent running,
DMAoff and exact run/launcher absence verified before explicit RELEASE.

Project channel count/event layout/master bytes are never padded in storage.
Only a private replay view adds silent voices. Four-track playback allocates no
padding. Active AmiGUS/MIDI routes remain refused, so this does not claim mixed
backend routing, physical-machine sound or realtime performance acceptance.
