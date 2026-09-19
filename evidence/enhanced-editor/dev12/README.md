# Channel controls and classic Paula mute/solo, dev12

The accepted main screen is retained. Control-R or a P/A/M header letter opens
CHANNEL settings in the existing upper control grid. Routes are exclusive;
Paula is limited to four assignments. Mute and solo are independent saved flags.
Multiple solos are allowed, with mute taking precedence. Existing note/sample
data is preserved when routes change. No MIDI connection or AmiGUS hardware call
is made by choosing a route.

One bounded journal now orders note, block and channel edits together. Channel
commands store before/after settings without spending event-change slots. Tests
cover interleaving, dirty/save state, redo preservation on no-op/refusal, eviction,
external-change conflicts and prevention of a fifth Paula route during undo.
Navigation is not undone. Project saves retain the existing version-1 format.

Classic four-channel Paula playback supports saved/live mute and solo. A shallow
playback-only copy clears those two flags for MOD snapshot encoding; the owned
volume output applies the channel mask. Exactly six generated volume writes are
hooked; other effect/timing code and vendor inputs are unchanged. Raw effect
volume is preserved for unmute, while display/test observations read the effective
volume supplied to the hardware register. The helper preserves registers and the
original condition flags. Strict disk MOD export still refuses saved mute/solo.

## Retained evidence

- `core-build.json`: final native binaries, toolchain and source identities.
- `host-tests.log`: all 23 host test groups, including sanitizers, persistence,
  shared history, unchanged main-screen golden, and incremental/full pixel checks
  across channel-panel edits/navigation/undo.
- `playback/`: native replay/ownership tests plus classic UI play/edit/save/reopen.
  Native tests include muted start, live unmute, solo/shared solo, mute precedence,
  strict export refusal and byte-identical source preservation after flags reset.
- `channels/`: actual native Intuition route/mute/solo commands, fifth-Paula refusal,
  undo/redo and no-op retention. Saved CHAN bytes, selection and file CRC are
  independently specified; all other bytes are unchanged. Reopened saves are
  identical. Classic UI replay observes live mute/solo/undo/redo and retained mute
  after restart. Switching to an unsupported route stops playback and all DMA.
  PTPatternTest executes the unified-history tests on the Amiga CPU too.

Screenshots are native emulator output. These tests observe replay state and the
values written by the volume hook; they do not establish listened audio quality.
New channel mouse targets are covered by the shared controller host tests; the
native channel workflow uses keyboard input. The original main layout remains
accepted, while the new secondary page has not received separate owner approval.

The emulator window was explicitly released by the AmiConnect task. Only the
exact private ProTracker profile was controlled. Guarded shutdown and fresh
process/disk/socket release checks are recorded in `emulator-release.txt`.

## Remaining boundaries

Mixed 1–16-channel Paula/AmiGUS/MIDI dispatch, CAMD, sampler integration and
existing-song resizing remain open. Route settings are editable/persistent, but
only supported classic four-channel Paula projects currently play. Sample audition
is independent of song mute/solo and stops on song edits. CIA-A timer-B fallback
execution is NOT RUN because Workbench already owns it; its vector is preserved.
Physical AmiGUS, ACA1234 performance and sound quality are NOT RUN.
