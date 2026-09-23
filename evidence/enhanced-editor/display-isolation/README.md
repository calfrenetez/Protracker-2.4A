# Settled display isolation — 2026-09-23

Two coordinated runs used the same disposable stereo24/192k project and shared
030 baseline. Both waited800ms before screenshots, then verified keyboard Escape
cancellation, unchanged serialized project, restored recent ENV, normal exit0,
DMAoff and exact run cleanup. Emulator explicitly released afterward.

- working/: current editor including preserved uncommitted prepared-display work,
  SHA25670e70ab347417e157fd80347e8c4534436b2eb412af2a2be1f203ac99781b4eb.
- committed/: isolated archive of351a8ca src/tools/vendor/tests, built using the
  same pinned compiler and existing pinned vasm, without uncommitted files,
  SHA256bb3d3e495071de0adb89ced326efb72563e03788d32b20649377d2db863266b2.

The settled working screenshot retains waveform gaps and corrupted lower controls.
These are absent from the committed-source screenshot. Therefore the corruption
is associated with the uncommitted display changes, not merely an early screenshot
or the sample-memory feature shared by both builds. This comparison does not
identify the precise failing instruction or prove every other screen correct.
The crowded cancellation/status presentation occurs in both, so remains a separate
readability issue. No claim of deliberate refresh-event acceptance.

Uncommitted display files were not edited, staged, discarded or overwritten.
The isolated build is at build/dev/visual-clean-351a8ca (not a release package).
Runner now accepts --editor and takes before/after settled screenshots. Fixture
and functional assertions are unchanged. Native isolated full build PASS and both
UI runs PASS functionally. No product source change or full host-suite rerun here.
No physical/listening/AmiGUS evidence. Shared startup and configuration unchanged.
