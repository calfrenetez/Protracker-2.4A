# Recovery configuration: complete values only

The production reader requests binary text and refuses empty/control-text or
possibly truncated values before storage access. Missing optional settings are
separate from failed reads; only an absent interval defaults to 300 seconds.
Intervals require decimal digits and explicit removable permission is 0 or 1.
A conservative spare byte handles both V36 and V37+ DOS return conventions.

Host ASan/UBSan test passes for both return conventions, missing default, empty
values, read failures, oversized valid prefixes, embedded NUL/newline/control
bytes, interval boundaries and removable-media policy. All invalid cases refuse
before opening storage. The native fixture uses private settings/time inputs;
it does not alter guest ENV or clock.

Shared030 native run render-files-1790557538724406000 passed: expected cleanup
probe RC20/zero owned bytes before functional launch; cumulative recovery RC0,
365 Fast allocations returned to zero and original source bytes unchanged.
Fixture: 97092 bytes, SHA256
3762b32623e0b070bd71038456711193883c5d163cce4979dbecd369b974d3b0.
Its archived native-build manifest predates a header-comment-only clarification.

Actual DOS environment reads were then qualified through the editor workflow in
shared030 run recovery-ui-1790557730740752000, under editor-ui/. Candidate262628
bytes SHA256 1ecaee823ff9829762c1ec7a6f2ffeda0c1a0ef2dfae3152b65fac5d7c81c552.
The selected editor/guard build has146 source hashes verified against the index.
Both prompts were inspected using fresh guest bitmaps before input. Decline,
restore, real30-second autosave, exact full-precision project save, source
preservation and normal exits passed. All five temporary ENV settings were
restored with exact presence/bytes. Both windows passed independently locked
running/identity/DMA-off/exact-path absence checks before explicit release.

No physical storage, real crash/endurance, positive AmiGUS, listening or deadline
acceptance is implied. The physical Amiga remained off and unprobed.
