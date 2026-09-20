# Native loop and slice editing, dev14

LOOPS and SLICES tabs extend the native sampler using the accepted main-screen
geometry, pinned bitmap font and raised controls. Forward/pingpong metadata,
loop removal and a baked crossfade are single shared-journal commands. Baking
changes PCM and loop boundaries together, publishes a forward loop that skips
its consumed head, and never applies a second runtime crossfade.

Manual markers and configurable AUTO SLICE operate on editable proposals.
APPLY publishes the complete marker set, CANCEL preserves project/redo, and
sample replacement/slot changes/sample undo discard stale proposals. Markers
never chop PCM. Every sample-version apply, undo and redo checks referenced
slice ordinals so no existing note silently acquires another start frame.
Unsafe changes are refused instead of automatically rewriting pattern notes.

## Validation

- `host-tests.log`: all 24 test groups pass, including ASan/UBSan sampler/controller
  checks. Added checks cover loop metadata, baked PCM/loop atomic undo, allocation
  rollback, no-op redo preservation, non-destructive marker commits, references
  added before undo/redo, same-count retarget refusal, exact metadata persistence,
  draft controls/cancellation/stale proposals, and full/cached display identity.
  The accepted main-screen golden is unchanged.
- `core-build.json` and `native-build.log`: final 68000/soft-float binary and source
  identities; the existing `-fbbb=-` compiler workaround remains enabled.
- `loops-slices/native-loops-slices.json`: actual native key input drives WAV
  import, forward/pingpong/off, crossfade bake and undo/redo, manual and AUTO
  proposals, editing/cancelling/applying, project save, reopen and normal exits.
  Independent host inspection verifies CRC, forward boundaries 32–2048, zero
  runtime-crossfade metadata, four marker frames [0, 512, 1024, 1536], exact baked
  signed PCM bytes and byte-identical resave. Native PTSamplerTest additionally
  runs the ownership, rollback, reference and persistence assertions on 68k.
- `sampler/native-sampler.json`: the existing native mono8/stereo24 import,
  selection-independent PCM edits, undo, WAV export, failed/cancelled import,
  protected existing destination and save/reopen regression passes on this build.
- Screenshots are real native output. New mouse targets/range operations are
  covered through the shared host controller; native workflows use raw keys.
- `emulator-release.txt`: coordinated ownership and final guarded shutdown,
  with fresh process/private-HDF/socket checks. Profile is private 68030/no FPU,
  2 MiB Chip/128 MiB Z3; it is not a measurement on the physical ACA1234.

## Boundaries

Editing/persistence is implemented. Pingpong, slice-trigger and high-resolution
preview remain unconnected, as do mixed AmiGUS/MIDI playback and CAMD transport.
Unsupported audition is explicitly refused. Classic-compatible sample changes
stop the existing private audio snapshot; failed/no-op operations preserve it.
Metadata-only sample versions still copy PCM within the existing 32 MiB history
budget. Proposals are not saved until APPLY. WAV export contains PCM only.
See `docs/ENHANCED_EDITOR.md` for controls and the remaining sampler work.
No physical AmiGUS, sound quality or ACA1234 performance acceptance is claimed.
