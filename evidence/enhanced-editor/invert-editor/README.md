# Native editor EFx export/bounce — 26 September 2026

Candidate built from committed b2be53d plus only the EFx native-controller and
build-input patch. Unrelated uncommitted display/presentation work excluded.
Canonical no-FPU/CRT/compiler-safety flags and source/header hashes are recorded.
Working checkout's editor also builds, but its display edits are not qualified.

Shared030 run invert-ui-1790462738704840000 PASS. The accepted classic layout
was inspected in editor-bounce.png. Through existing controls, requested-file
cancellation preserves the project; stereo24 WAV and both17280-frame stems
exactly match host bytes. Selected-track bounce saves exact expected PCM in
new sample032, while all31 original sample records are byte-identical to the
baseline project. Undo/redo after save returns to revision1/clean. Native editor
exits normally. Prior recent-prefix ENV existence/bytes restored and verified;
all4DMAoff and exact owned cleanup pass. Window explicitly released.

This is emulator software/file/UI evidence. Physical A1200 timing, analogue
sound and physical AmiGUS acceptance are not established. Queued Studio EFx
remains unsupported. Other documented EFx input restrictions still apply.
