# Row-range wavetable session validation — 27 September 2026

The injected song session now consumes silent pre-roll without cache uploads or
voice callbacks, restores its exact first emitting snapshot before elapsed output,
and resumes normal dispatch. Full sequence plus restore preflight runs before
source pins/output. Only active restored samples and future emitted triggers are
pinned. Masters retain source precision and enhanced-save bytes.

Coverage: absent restore capability; late unsupported trigger with no side effects;
fractional Q32 cursor parity with the renderer; samples finishing during pre-roll;
future triggers; first-row range; single final emitting interval; cancellation
before output; uncertain restore and pending natural stop retaining resources
until confirmation; exact saved project bytes and complete allocation cleanup.

Validation:
- Dispatch ASan/UBSan fixture PASS (6.172s).
- Three index-export editor wavetable, legacy guard and Studio ASan/UBSan fixtures
  PASS (26.336s), excluding unrelated working-tree display edits.
- Native pinned-compiler build and native editor syntax check PASS. All 149 build
  dependency hashes matched staged sources (generated font checked separately).
- Binary PTExecEditorWavetableTest: 283344 bytes, SHA256
  `6ca6c8fb1519fe9da4ab3e1c4e4a64b839f62ab25d7c7a9102da768915514e7f`.
- Fresh AmiConnect-coordinated shared030/DevBench run
  `render-files-1790480674092320000`: RC0 within the 90-second bound; 206 Fast
  allocations, zero owned bytes, budget refusal without Chip fallback.
- Shared lock/live target guards, all four DMA off and exact owned cleanup passed.
  Explicit release sent to AmiConnect. No recovery hold.

Initial host fixture setup cleared an empty sample descriptor to invalid zero
metadata; corrected by restoring its original descriptor before save assertions.
Final host/build/emulator results above are for the corrected staged candidate.

Limits: injected callbacks and software ownership only. No native card register
lowering, hardware phase/stop fence, real-time clock, audible output or physical
AmiGUS acceptance. Existing editor development package is unchanged. The prior
renderer timeout remains separate failed-deadline evidence, not part of this pass.
