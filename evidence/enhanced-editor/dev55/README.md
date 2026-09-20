# Dev55 pinned unused-command compatibility

Software/emulator milestone PASS; physical acceptance remains open.

The pinned2.3F replay dispatches8xx to PerNop/SetBack, restoring the stored period
on fresh and effect passes, with no panning interpretation. E8x dispatches to
an empty KarplusStrong routine and preserves output registers unless another
replay action writes them. No synthesis or destructive sample edit is invented.

Host all-parameter checks across16 tracks passed0.818s;68 existing/extended host
tests passed123.148s before adding retained native-oracle regression. Refusal
fixtures for render, bounce, stems and their old native scripts now use EF1,
which remains unsupported, rather than commands newly supported by the renderer.

Physical A1200/AmiGUS acceptance is separate and not claimed.

Native unused1789945548816869000 passed27 executions in106.551s: six fixtures
each captured twice identically, corresponding m68k stored/output pitch and
reference PCM comparisons, retained baseline and boundary/refusal checks.
The new retained-trace host regression then passed4.678s. This is69 distinct
passing host tests (68-test suite plus the newly added native-evidence check),
not a claim that the earlier full-suite log included the new test.

Guarded QUIT and fresh no-process/HDF/socket/original-launcher release checks
passed at23:08:09UTC; AmiConnect explicitly notified. No current claim remains.
8xx is not modern panning and E8x is not Karplus synthesis. E0/EF remain refused.
