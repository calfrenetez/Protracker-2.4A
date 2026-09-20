# Waveform range and explicit format conversion, dev15

RANGE provides independent waveform zoom/pan, selection zoom and exact numeric
frame endpoints. Navigation does not edit PCM, history or the selected range.
Waveform clicks, markers and loop brackets share the zoomed coordinate system.
Modal number entry rejects invalid/overflowing values and cannot invoke other
editor actions. Sample slot/frame-count changes reset the viewport.

FORMAT proposes explicit whole-sample bit depth and rate until APPLY. Conversion
preserves mono/stereo, scales marker and loop positions, and publishes PCM and
metadata as one shared undo resource. Existing slice ordinals retain their
corresponding scaled markers; collapsed markers are refused. Undo restores the
original precision and all original frame positions exactly.

## Evidence

- `host-tests.log`: all 24 host groups pass. Added ASan/UBSan coverage checks
  zoomed click mapping, single-frame view, pan limits, tab boundaries, exact entry,
  overflow/invalid/cancelled entry, modal-action isolation, unchanged history,
  full/cached render identity, stereo24 conversion, fixed expected output values,
  scaled referenced markers, allocation rollback, refusal preserving redo,
  bit reduction/clipping, exact undo and serialized persistence. The main-screen
  golden remains unchanged.
- `core-build.json` / `native-build.log`: final source/compiler/binary identities.
  Build remains 68000/soft-float with the existing `-fbbb=-` workaround.
- `range-format/native-range-format.json`: actual Amiga raw-key events enter
  exact endpoints, zoom/pan, reject invalid endpoints/rates, set a loop, choose
  rate/precision, convert, undo, refuse a marker-collapsing rate and redo. Independent
  host inspection checks CRC, exact linear interpolation/precision values,
  scaled loop and marker frames, byte-identical save/reopen and two normal exits.
  Native PTSamplerTest also runs sample/loop/conversion assertions on 68k.
- Screenshots are real native output. New mouse targets and zoomed two-click
  selection are exercised in the shared host controller; native input used keys.
- `emulator-release.txt`: explicit Amiberry coordination, exact-profile guarded
  QUIT and fresh process/HDF/socket release checks.

## Quality and hardware boundaries

Rate conversion is explicitly LINEAR without an antialias filter; precision
reduction rounds/saturates without dither. This is not the finished high-quality
classic-song conversion engine. Filtered conversion is subsequent work. PCM
conversion never automatically downmixes stereo or silently drops markers.

Enhanced preview, pingpong/slice-trigger playback, mixed AmiGUS/MIDI and CAMD are
still open. Physical AmiGUS, sound quality and ACA1234 performance are NOT RUN.
The profile uses private 68030/no-FPU/2 MiB Chip/128 MiB Z3 emulation, not hardware.
