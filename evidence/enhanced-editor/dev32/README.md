# dev32 — editor reference WAV export

DISK OP. → RENDER WAV / Control-Shift-W exposes song/pattern scope, track mask,
44.1/48 kHz, 16/24 bit, gain and startup lead-in. Native export stops playback,
preflights before the requester, and permits Escape during checking/mixing/byte
verification. Export does not consume undo history or clear unsaved project edits.

## Evidence

- `host-tests.log`: all 42 host checks passed in 53.273 seconds.
- `editor-test-final.log`: final controller/view sanitizer check passed; main-screen
  SHA256 remains `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.
  291 cached/full redraw steps include the render page and numeric entry.
- `render-file-test.log`: final file publication check passed, including monotonic
  actual-byte verification progress through the final 100% callback.
- `native-render/native-render-ui.json`: full native workflow passed in 233.013 s.
  Requester, mixing and verification cancellations preserve edits and leave no
  output/staging. Dirty song export and clean pattern export match the host WAV
  exactly. Finetune is refused before the requester. Undo/save restores the exact
  baseline PTG. DOS true24, corruption, destination-race and cleanup tests pass.
- `native-settings/native-render-ui.json`: final packaged editor validates long
  mask entry, overflow refusal, cancellation and exact project preservation.
  Its only change from the full-workflow editor is displaying numeric mask entry
  without the TRACKS prefix, keeping ten entered characters inside the button.
- Screenshots were inspected: settings show the chosen 44.1 kHz/16 bit/100% gain;
  the final numeric-entry screenshot shows all ten characters inside their cell.
- `core-build.json`: final compiler/runtime/source hashes and all native binaries.
- `amiberry-release.json`: fresh process, private-disk and socket release checks.

Song/pattern WAV: 33516 frames, 134108 bytes, SHA256
`67feddde8c6b507d21625318eef14c20961025476f0a6e2ef430e0b3d37b2ec8`.
Final PT24GEdit: 182092 bytes, SHA256
`85577631413d1e77ca822ece2b0ae91348c056d48056275f5a63e78650f83096`.

## Corrections and scope

The initial host test had a signed/unsigned assertion warning; it was corrected.
An initial native run proved exports and cancellations, then failed to navigate
with the emulator mouse helper. Only that owned harness was interrupted; its
finally block restored the launcher and released the guarded emulator. The final
workflow uses keyboard navigation. This was a harness-coordinate issue, not an
application crash. Logs are retained.

Native testing also identified verification progress counting the preliminary
scan. It now reports frames already compared, without resetting, and reaches
100% before publication. The final file/controller checks were rerun after these
changes; no claim is made that the earlier full-suite log used the later sources.

This remains the explicitly supported reference-effect subset, with ideal-BPM
clock and PCM-rate pitch policy. It does not establish full classic-effect/CIA
parity, sample-slot bounce, batch stems, listening acceptance or physical
AmiGUS/A1200 performance. No physical hardware was accessed.
