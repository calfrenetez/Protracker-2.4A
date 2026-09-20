# dev40 — initial-segment to repeat-loop playback

`pt_voice_init_segment` plays a nonempty initial PCM range, then a separately
bounded forward-repeat range. Ranges may be disjoint or overlap. Fractional
overshoot is carried into the loop, and interpolation crosses the initial end
to the repeat start. Once looping, the old initial endpoint no longer affects
interpolation. Large Q32 steps wrap without overflow. PCM is borrowed unchanged;
invalid ranges preserve the voice. The ordinary initializer retains its policy.

## Validation

- All 50 host checks passed in 79.209 seconds with sanitizer coverage.
- Native run `segment1789930535396415000`: both new segment and original voice/mix
  tests passed in 27.784 seconds. Native output matches host output exactly.
- Cases cover earlier/later repeat ranges, overlapping ranges and endpoints,
  fractional interpolation joins, one-frame loops, invalid ranges and five step
  trajectories, including UINT64_MAX. An independent Python unbounded-integer
  oracle checks the trajectory hashes across 100 frames each.
- Original native voice/mix partition result remains `990c42f0`, clipped40,
  covering existing forward/pingpong, stereo24, 16-voice mix and muted progression.
- Review found and fixed an overlapping-endpoint interpolation edge case; the
  final complete regression and native runs include its new test.
- Current source and native-test hashes match `core-build.json`.
- Guarded cleanup at 18:56:29 UTC verified no process/private-HDF holder/socket
  and exact launcher restoration. AmiConnect received explicit release.

## Acceptance boundary

This is the voice primitive needed by the dev39 offset ranges, not newly enabled
9xx rendering. Per-track offset memory/ranges, renderer integration and PCM checks
against the native trigger/loop evidence remain next. The accepted main screen
is unchanged. No physical hardware was accessed; this does not prove Paula
analogue audio or AmiGUS/A1200 performance/listening acceptance.

## Binary identities

PT24GEdit: 187164 bytes, SHA256 `95294393df512ab18d65e06c413aae465d5736eb4a2dc0cc988fab8fde46fdf5`.

PT24GRender: 62448 bytes, SHA256 `8a1da0e340a6a300c497fadbca3dc2457fa857ba2184e304a85bdd0901398bb6`.

PTVoiceSegmentTest: 32216 bytes, SHA256 `57d1f408466fd080cd96c44506815ba42f965c2b5c102c6fadf11fb0f18860b8`.

PTVoiceTest: 37096 bytes, SHA256 `afc6626ac6042563f0ac60aa38b0a3e618ddffd6cb61fc88bfa0a6bacb85b9a3`.
