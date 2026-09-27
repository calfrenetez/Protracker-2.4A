# EFx instrument changes with E9, ED and9xx

Reference captures: invert-shared-1790468637618716000, shared030, two executions
of each fixture. Every execution returned0 and native-stop. All four DMA channels
were off before exact owned cleanup and explicit release. No lifecycle, profile
or physical operation. PTInvertTraceTest and its pinned-replayer build manifest
are unchanged from the earlier handoff capture.

All fixtures start sample1 at period381 with EF8, switch to sample2 on row1,
return to sample1 with EFf on row2, then stop with F00. Sample2 is either a16-byte
loop or a512-byte one-shot whose stored first word is deliberately nonzero.

- E92 without a note restarts sample2 on ticks0,2,4.
- ED3 with period480 selects the new loop immediately but keeps period381 until
  the delayed trigger on tick3.
- 901 with period480 starts a one-shot at byte256 for256 bytes; the second offset
  application leaves the stored length at2. For the looped sample, the offset
  exceeds its initial80-byte region and triggers the two-byte fallback instead.
- Every variant inherits EF8 accumulator96 at the handoff and follows
  96,112,0,16,32,48 across that row. The fresh instrument load resets the cursor,
  not the accumulator or speed.

Raw logs preserve allocation-relative pointers. Canonical traces map each
allocation from its stable loop pointer and MOD loop offset. Start/retrigger
addresses, cursor changes, clocks and PCM bytes are not erased or resynthesized.
Tests intentionally damage those fields to confirm differences remain visible.

Host validation (host-tests.log):8 sanitizer-backed tests pass, covering the six
new and four earlier fixtures. A separate PCM oracle compares all17280 stereo24
frames offline and with pulls1/17/256. Actual WAV/stems/grouped stems, bounce,
cancellation, undo/redo and master-save/reload workflows remain exact. The9xx
one-shot return happens at a tick boundary; the other cases include a transfer
inside a tick. Production renderer/source code is unchanged at this milestone.

This is bounded software compatibility evidence. It does not establish Paula
analog fidelity, A1200 performance/stability, AmiGUS behavior or all possible
combinations of effects. Unselected shared mutation clocks during concurrent
cross-instrument changes remain the next audit. Existing format/slice/rate guards
and native Studio output restrictions remain unchanged.

Native core qualification: invert-handoff-1790468827340788000 on shared030
completed within the reserved420 seconds, all six RC0. Each case reports41
Fast/not-Chip allocations and zero final owned bytes, including budget refusal.
The exact44 compiled dependencies and compiler/runtime identities are in
`native/build.json`; every production dependency matches committed source.
Binary SHA256:01fbf32671b58eab7a0c8c304d905ea7747ea5492c1ce6e89ebd5814deb76673.
All four DMA channels were off before exact owned cleanup, absence verification
and explicit release. No deadline extension, retry or physical operation.
