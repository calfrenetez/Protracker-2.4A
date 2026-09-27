# Sampled clock adapter — 27 September 2026

Checked rational elapsed-tick conversion and injected song/editor reader adapter.
Host ASan/UBSan instrumented dispatch passed (7.515s); final staged editor,
guard and Studio suite passed (28.946s), including converter overflow through
adapter and clock-bound disposal. Native main syntax/build passed. All168 staged
inputs/generated font and the binary hash were independently verified.

Shared030 run render-files-1790504530038403000 passed within the90-second guard,
RC0. Binary377316bytes SHA256
3899b5b32d734268bdd73f73c066c9da2be4c0c0833ff659ff69bb2ac49da401.
1178 Fast allocations, zero owned bytes, no Chip fallback. All4Paula DMAoff,
exact run/launcher removed and absence independently verified. Coordinated with
AmiConnect and idle other-task snapshots; resource explicitly released afterward.

Synthetic clock readers and device callbacks only. No native clock acquisition,
event-loop wakeups, native enhanced PLAY/card output or physical timing/audio
acceptance. Unrelated dirty display/harness files excluded from staged builds.
