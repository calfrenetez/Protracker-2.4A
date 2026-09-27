# Confirmed-stop editor mutation barrier — 27 September 2026

The editor wavetable binding owns a veto-capable change barrier. Pending/failed
stops retain the session, masters, playback leases and editor/document storage.
Retry after confirmation permits edits, imports, undo/redo, replacement and
teardown. Cursor navigation remains available; the selected-channel cursor is
excluded from the song metadata consistency check. Existing Studio and legacy
synchronous hooks remain supported and cannot be attached alongside this owner.

Host ASan/UBSan: all three editor-wavetable, legacy guard and Studio fixtures
passed (25.946 seconds total). The wavetable fixture verifies pending and failed
stops, mutual attachment exclusion, exact 24-bit enhanced-save preservation,
reverse/undo/redo retries, disposal/detach retention and complete final release.
Project replacement is protected through the prepare-change API; the native
New/Load/quit/bounce/Stop call sites were syntax-checked, not UI-driven here.

The native fixture was built from a Git-index source export excluding unrelated
local display work. All 149 dependency hashes match that export/index; the
bitmap font source and compiler/runtime inputs are pinned in build.json.
Binary: 269456 bytes, SHA256
`c42c323cb5d5ab31e38de32726e65b8067cf84c469057ea77723c14675b1fd16`.

Shared030 run `render-files-1790478371042795000` returned 0 within the 90-second
runner deadline: 146 Fast allocations, zero final owned bytes, budget refusal
without Chip fallback. Shared lock and live guards were used. All four Paula
DMA channels were off and exact run files were cleaned. AmiConnect received
explicit release. An additional release notification to Scott was blocked by
automatic approval review because messaging authorization names AmiConnect;
no further message to that chat was attempted. No emulator hold remains here.

These are injected-driver software ownership tests, not real card output or
stop-fence proof. Native PLAY does not instantiate this adapter. No physical
operations, device MMIO, audible acceptance or full native-editor runtime test
were performed. The existing d7ca8e7 native editor package is unchanged.
External owners must honor prepare/dispose refusal and detach before freeing
or reinitializing editor storage; a pending stop cannot be force-freed.
