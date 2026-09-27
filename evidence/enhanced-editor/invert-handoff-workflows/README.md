# EFx handoff export, bounce and master-save workflows

Application source335f98146b7fa3c709a173690e893a522c480021, built from a committed
archive. Unrelated local display edits are excluded. Build manifests identify the
canonical compiler/runtime, sources/headers/generated objects and exact binaries.

Host tests use all four pinned reference handoff fixtures under `invert-handoff`.
They exercise actual new-file WAV export, two stems, grouped stems, verification
cancellation, existing-file refusal, bounce cancellation after a handoff, bounce
commit, undo/redo, budget refusal preserving redo, and project-file save/reload.
Every resulting audio frame matches an independent model of the reference trace;
the model reads the recorded loop PCM, period/volume and repeat pointers without
calling the production renderer. Each result contains17280 stereo24 frames.
The empty second track produces a silent stem. Earlier shared-mutation fixtures
continue to cover multiple audible channels and unselected EFx clocks.

The saved/reloaded bounce exactly matches the WAV. Omitting only its appended slot
from serialization leaves the original project byte-identical; saving after undo
also yields exactly the pre-bounce file. Original nonzero one-shot first bytes are
preserved. Cancellation leaves no partial output/staging, no changed project or
undo history, and no tracked allocations. Host regression:2 tests PASS in6.166s,
including all four transaction cases and separate checks at the editor's default
8287Hz sample rate /50% render gain. See `host-tests.log`.

`host-reference/` uses sanitized host CLI tools from the same committed application
source and the `invert_swaponce.mod` capture fixture. The files are retained for
byte-exact native UI/CLI comparison; their PCM also matches the independent trace
model at the default sample rate (50% UI gain,100% CLI qualification gain).

Native editor run `invert-ui-1790467813195970000` PASS within the480-second
window: WAV requester cancellation, exact full WAV and two stems, exact24-bit
bounce into sample32, all31 source sample records unchanged, save/undo/redo,
normal RC0 exit. Screenshot was reviewed; accepted classic layout retained.
PT24GEdit SHA256:31c59abe91fe57f289ca600b0ad3d992c82ee7bb8dd8c54b49f135ff6fc796f3.

The prior recent-prefix ENV presence/bytes were backed up, run-owned isolation
verified before launch, and exact restoration checked after exit. All4DMAoff,
exact owned run/launch cleanup and absence verified; explicitly released to
AmiConnect and Scott. No audio playback, lifecycle/profile or physical operations.

Native CLI run `render-files-1790468049861883000` PASS within240 seconds:
RC0, exact two17280-frame stems against independently checked host bytes, source
unchanged, existing destination refused RC20 without replacing files. Renderer
SHA256:65543286d9c04df557294293c1e6b35e9cc6e9fc45a05ca1f0814cad052696b7.
All4DMAoff and exact owned cleanup/absence checked; explicitly released. Neither
of this continuation's application windows required a deadline extension or retry.
These are software/emulator workflow checks; physical audio, A1200 performance
and physical AmiGUS remain unqualified.
