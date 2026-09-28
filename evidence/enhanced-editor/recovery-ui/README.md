# Native recovery editor integration

Final shared030 run `recovery-ui-1790557081632882000` passed with the staged
editor: 262480 bytes, SHA256
`01641ef5e61d469004cafd772985c333b90772eae5ffe605dbb31a961c06a166`.
The final selected build produced the editor and guard harness; both binary hashes
and all 146 recorded source hashes were verified. Earlier complete staged builds
produced all 139 executables. Unrelated unfinished display work was excluded.

`final-candidate/` contains final build identity, guest screenshots, logs and
results. Both recovery requesters were inspected before input using fresh Amiga
bitmaps. Decline preserved the source and retained recovery copy. Restore marked
the project unsaved; a real 30-second idle interval created its own snapshot.
Save matched the expected project byte-for-byte, including every original sample
record; the source was untouched and only the live session snapshot was discarded.
Both editors exited normally. Five temporary ENV values were restored with exact
presence and bytes. Independent running, DMA-off and exact path absence checks
passed before the shared window was explicitly released.

Amiberry IPC captures sometimes returned black while fresh AmigaBridge bitmaps
showed the correct requester. `guest-capture-comparison/` preserves that evidence;
the initial editor presentation change is not claimed to fix the capture issue.
Final recovery status strings fit the existing 30-character field. These checks
qualify recovery behavior, not every unrelated display feature.

The native seed helper created an actual production snapshot, with a private
fixture timestamp, and released all 10 tracked Fast allocations. It did not crash
the editor or change the guest clock. Earlier failed input/inspection windows are
preserved under the named subdirectories. The initial successful candidate is
retained below for traceability. Host regression results are in `host-regressions/`.
No physical Amiga, positive AmiGUS, listening, real crash or timing acceptance is
implied. The physical Amiga remained off and unprobed.

## Earlier successful candidate


Shared030 run recovery-ui-1790555656732573000 passed the real native editor
workflow with candidate262556 bytes SHA256
ac78da19557826caa2d5a3e695f2f311eb372a82f371805230f33f5f231265d4.
The immutable staged build produced139executables; all binary byte counts/hashes
and293 source hashes were independently verified. Unrelated unfinished display
work was excluded. A final status-text shortening is built/qualified separately.

The seed helper95928bytes SHA256
51b577c8220c09a414abb564b9645759b4ce9e51c7c5f8c24eb445a32ab81067
created an actual production recovery snapshot and an expected restored project,
then exited normally with10Fast allocations returned to zero. It simulated the
snapshot timestamp privately; it did not change the guest clock or crash the editor.

Both requester choices were visibly inspected. Keep current left the original
and recovery copy untouched. Recover restored the full-precision project and
marked it unsaved. With the real editor timer and30-second interval, idle
work was snapshotted into a separate owned session. Saving produced the expected
project byte-for-byte, preserved all original sample records, left the original
source untouched, and removed only the current session's owned autosave. Both
editor instances exited normally withRC0. Old crash copies were not adopted or
deleted. Existing layout was retained; the long recovered-status text is shortened
in the subsequent candidate because the screenshot showed it wrapping.

All five app-specific temporary ENV settings were backed up before mutation and
restored with exact presence/bytes before cleanup. Runner and independently locked
running/identity/DMA-off/exact-path absence checks passed; window explicitly released.
Prior input/inspection failures remain under the named subdirectories. No physical,
real AmiGUS, listening or timing-performance acceptance is implied. This verifies
snapshot-based recovery; no deliberate machine crash/reset was performed.
