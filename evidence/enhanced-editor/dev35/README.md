# dev35 — normal and fine reference pitch slides

1xx/2xx and E1x/E2x now work in the shared reference renderer used by native
WAV export, the command-line renderer and atomic sample bounce. The pitch core
retains a 16-bit stored word separately from its latest playback-register write.
This preserves native wrapping arithmetic, 12-bit slide masks and limits,
full-word PerNop/SetBack restores, zero parameters and delayed-row effects.
Changing pitch retains voice phase and fractional position.

A triggered zero playback period is refused during measurement before any sink,
WAV staging or sample append. Reports remain unchanged on this refusal. This is
an explicit remaining reference limitation, rather than an invented sound rate.

## Validation

- All 45 host checks passed: 44 existing regressions in 61.510 seconds and the
  new pitch/native-trace PCM comparison in 3.692 seconds, with sanitizers.
- Native run `pitch1789925188600209000` passed all 30 executions in 102.024 seconds.
- Seven native fixtures provide 109 captured ticks per pass. Both captures match
  in every byte; portable host and m68k pitch state match every stored/output word.
- Six fixtures verify every one of 60480 true24 ramp frames against captured
  periods using an independent unbounded phase oracle. The seventh checks that
  a zero period fails before sink calls or report mutation.
- Hand-checked boundary cases cover all 16 track indices, upper/lower limits,
  word wrap, masked output and zero results.
- The separately instrumented baseline fixture preserves all original 36 fields
  byte-for-byte against dev28. The replay adapter, original flow instrumentation
  and native Paula test binary are unchanged. The trace harness now permits a
  compile-time record size; this shifts its diagnostic failure line numbers.
- Period-write wrappers preserve registers and MOVE.W flags, including X.
  The 52-byte diagnostic is never linked into the shipping editor.
- A final comment-only header rebuild produced identical binaries to those tested.
  `core-build.json` records current source/compiler/runtime/binary identities.
- The main-screen golden remains
  `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.
- `amiberry-release.json` verifies no process/private-HDF holder/socket and restored
  launcher after guarded shutdown. Ownership was explicitly returned to AmiConnect.

## Limits

The reference profile still uses raw note periods, sample rate at period428 and
ideal BPM intervals. Native note-table quantization, finetune, zero-period sound,
remaining effects, selected-row bounce, batch stems and enhanced live playback
remain unfinished. These tests establish register/PCM software behavior, not
analogue Paula sound or physical AmiGUS/A1200 performance/listening acceptance.
No physical hardware was accessed.

## Binary identities

PT24GEdit: 186100 bytes, SHA256 `898440cf54559b82a1a5407199efa5ea1dd9f1a6f5818dac31b38aa32af27977`.

PT24GRender: 61384 bytes, SHA256 `4042de319f3ad82a5bbc12b0ce8b2149306ac8cecb2f87548624b17d96afd296`.

PTPitchTest: 52904 bytes, SHA256 `f97e931da5b212be49b3e4f3a7618946ada8ade7adaf8ea09f472ef560bb1f7b`.

PTPitchRenderTest: 60608 bytes, SHA256 `359a78a822ac174067f82db1eda9e6bbbfed1c5fd4e4dd9d8e288c3a58c67bae`.

PTPitchTraceTest: 60764 bytes, SHA256 `0417460e44bc436b7bca0547f75af8b90c05fe57fb29e52235e257a87f85b6fa`.
