# Interactive preview cancellation — 2026-09-23

Functional result PASS, visual/refresh result NOT ACCEPTED.

Generated a disposable96000-frame stereo24-bit192000Hz project. In the native
editor sampler, F8 entered conversion; Escape was logged at5/4144 output frames
in the first run. The UI reported cancellation with revision0/dirty0. DMA remained
off, subsequent saved project was byte-identical to the input, and editor exited
normally with return0. Logs, hashes and screenshot retained for both runs.

The initial run used default recent-file storage and added its disposable input
and saved paths to the emulator's existing application recent list. This side
effect is recorded, not claimed undone. The harness was then changed to use a
run-local recent prefix, preserving/restoring the former ENV override including
absence. The rerun verified isolated use and restoration, exact project identity,
normal exit and DMA off. Run-owned files/script removed and emulator released
explicitly after each run. No startup/restart or physical operation.

Screenshot inspection exposed status text overlap and lower-screen artefacts in
the current editor build, which contains unrelated uncommitted prepared-display
work. Their origin/timing has not been isolated. Do not call this a clean visual
pass, a proven refresh-event test, or a clean committed editor release candidate.
The product display work was preserved. The current test proves keyboard-driven
cancellation and persistence/cleanup only. Deliberate refresh testing remains open.

Reproduce the fixture:

```
cc -std=c99 -O2 -Wall -Wextra -Werror -Isrc/core tools/make_preview_fixture.c src/core/project.c src/core/pcm.c src/core/channels.c -o build/host/PTPreviewFixture
build/host/PTPreviewFixture build/dev/preview-cancel.ptg
```

Build native editor with pinned core build; reserve shared030 with AmiConnect,
then run shared_infra_preview_cancel.py using shared infra Python. Guarded runner
uses existing shared transport/lock and a120-second workflow deadline. On an
unconfirmed exit it preserves files for guarded recovery; no guest reset/kill.
Production code unchanged this milestone; previous full105-test pass remains in
cancel-paula-preview. Fixture compiled with strict warnings; runner syntax checked
and interactive rerun passed. No physical/AmiGUS/listening acceptance.
