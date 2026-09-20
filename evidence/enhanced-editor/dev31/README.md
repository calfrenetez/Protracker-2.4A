# Streaming reference WAV utility, dev31

`PT24GRender` integrates the shared flow/timeline/voice/mix primitives into a
bounded native/host WAV export utility. Supported-subset preflight precedes output.
It supports song/pattern/selected-track stereo16/24 export at 44.1/48 kHz with
explicit gain, startup trim/retention, slice bounds and global mute/solo. It
retains the outgoing row before F00 or a same/backward position transition ends
one pass. Unsupported MIDI audio/pitches, finetune, instrument-only events,
crossfade metadata and effects are refused. See `docs/REFERENCE_RENDERER.md` for
precise profiles, effect subset, pan policy, budgets and limitations.

WAV publication uses uniquely owned same-filesystem staging. It streams at most
256 stereo frames per block, closes output, re-renders and compares every staged
byte plus exact file end, then publishes by POSIX link/Amiga DOS no-replace Rename.
Existing/late-created destinations and source PCM remain unchanged on failure.
Progress distinguishes analysis, mixing and verification; cancellation discards
staging. The CLI has no interactive cancellation key yet; that callback is ready
for the subsequent editor integration.

All 42 host checks pass in 54.226 seconds. ASan/UBSan tests verify exact PCM16/24,
startup trim, complete pattern endings, B00, delayed phase continuity, slice bounds,
volume/velocity/note-off, mute/solo, excluded-track global commands and unsupported
preflight. They verify frame/tick limits, cancellation, sink failure, exact true24
WAV bytes, no overwrite, a destination created during verification, deliberately
corrupt staging and cleanup. A process-local file-size limit causes a real short/
failed write and proves normal cleanup without affecting the parent process.
Invalid CLI settings create no output. The final file and flow tests were rerun
after a cross-compiler cleanup-format warning and counter assertions respectively;
those logs are retained separately.

The initial native compile caught misleading indentation on the CLI cleanup line;
statements were split and the complete cross-build passed warnings-as-errors.
Both initial and final build logs are retained. `core-build.json` identifies the
compiler/runtime/flags, all sources and native binaries; it now also hashes the
build script itself. `PT24GEdit` is still byte-identical to dev27..30:
`dfa0b3c42c4099705650a0a279711b6e25e7ef0e3e791870c56bac1ab9685888`.
The accepted main-screen bitmap golden remains unchanged.

Native run `render1789912889351473000` passed seven CLI executions in 234.263
seconds on the emulated 68030. Core render assertions and the actual DOS save/
corruption/cancel/destination-race assertions passed. Song and pattern exports
at 44100/stereo16 have 33516 frames, 44 ticks, no clips and exact host byte parity:
134108 bytes, SHA-256 `67feddde8c6b507d21625318eef14c20961025476f0a6e2ef430e0b3d37b2ec8`.
The native stereo24 test file contains 960 frames; every sample byte was checked
independently. It is 5804 bytes, SHA-256
`1ad3dc3bd76ca9d7084a0b7c858ec5a83aed6db9ca7665a63750ddb5bc52ded3`.
Existing output was preserved, invalid/unsupported outputs were absent and no
staging files/directories remained. These synthetic WAVs are regression fixtures,
not a listening/hardware acceptance set.

PT24GRender SHA-256:
`b21b963a2a82c2ab6d7367117a3cea5e0e1a4a700a9b023f1465d3bde9044355` (60328 bytes).
The renderer never starts a hardware audio device. All DMA channels were off;
launch restoration, guarded shutdown and fresh process/private-HDF/socket checks
proved explicit release to AmiConnect. No physical A1200/AmiGUS was accessed.

Reproduce with `make test`, `make core-tests`, reserve the emulator, then run
`python3 tools/test_render_emulator.py`. Host-only CLI build is `make renderer`.
This provides a deterministic supported-subset reference utility; full effects,
CIA/analogue equivalence, editor rendering/bounce and physical acceptance remain
separate gates. Native timing here is a bounded test duration, not a claimed
real-hardware throughput benchmark.
