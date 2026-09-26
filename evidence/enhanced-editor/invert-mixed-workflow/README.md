# Mixed EFx editor and renderer workflow — 27 September 2026 local

Application source: 91dc9b93c3317207e9e8b6b8ef5e5eb85ce322da, built from a
committed Git archive. Unrelated uncommitted display changes were excluded.
Canonical m68000, nix20, soft-float, -Os, -Werror and -fbbb=- flags were used.
Compiler/runtime/source/generated-object hashes are in the build manifests.

- PT24GEdit: 8ba241a96bc8dcdd8e8157dd8a9c177b908e43daf85c19fb2f312365df6b686b
- PT24GRender: 7699a6e0aa5f1b27772a0a1176d7fdd845c37d5fe7527d0552dfa4877b469ea9
- Fixture: b526fc0db6a52b547d06d4c0e3f9e37c468f6e5be4096983c5cbb6e856a6848b

The fixture's first instrument is a one-shot whose original first word is 11a3;
its second is a separate forward loop. An unselected third channel applies EF8
to instrument 1 while selected track 1 applies EFf/EF0. Host WAV, stems and
project references are retained. Their generation used sanitized host tools
compiled from the same committed application source. The CLI regression also
checks stem summation, source identity, existing destinations and budget refusal.

Native CLI run render-files-1790464209247669000 passed with RC0, exact two-stem
host parity (17,280 stereo24 frames each), unchanged source, and RC20 for the
existing output directory. No partial staging remained.

Native editor run invert-ui-1790464330619363000 passed requester cancellation,
exact full WAV and two stems, exact selected-track stereo24 bounce into sample32,
project save, all31 original sample records unchanged, undo/redo and normal RC0
exit. The saved master retains its original nonzero first word. Screenshot was
reviewed; accepted classic layout is retained.

Recent-prefix isolation was verified before editor launch; the prior ENV setting's
presence and bytes were restored and checked afterward. Both emulator windows
verified completion, all four audio DMA channels off and exact owned cleanup,
and were explicitly released to AmiConnect/Scott. No reset/config/physical actions.

These are software/emulator workflow checks, not physical Paula sound, A1200
performance/stability or AmiGUS acceptance. Queued Studio EFx remains unsupported.
