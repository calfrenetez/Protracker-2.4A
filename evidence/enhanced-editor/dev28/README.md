# Native reference traces and portable flow core, dev28

The shipping editor is unchanged from dev27 (SHA-256
`dfa0b3c42c4099705650a0a279711b6e25e7ef0e3e791870c56bac1ab9685888`).
The diagnostic uses a separately generated and linked replay object; the pinned
vendor input and ordinary replay adapter are unchanged. The existing visual
golden remains `87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb`.

`native/` contains 16 synthetic MODs, complete binary traces, both matching text
captures, fixture/trace hashes and environment details. Run
`flow1789909763524316000` completed 32 captures in 76.149 seconds. Each record is
36 bytes; its first 30 capture control flow and the last six native DMA/volume
observations. Disabled setup interrupts bypass recording; F00's last tick is
included. Reaching the exact capacity stops diagnostic recording with a distinct
budget reason. Storage is allocated before replay starts; no allocation/printing
occurs in the ISR, and the interrupt is removed before reading/freeing it.

`src/core/flow.c` implements allocation-free row/tick control over an immutable,
validated project. Classic mode preserves 4-track/128-position behavior and the
initial 6/125 lead-in. Extended mode uses 1..16 tracks/256 positions, processing
commands in ascending track order even on muted/MIDI routes. It distinguishes
played/next cursors, fresh/delayed processing and explicit stop/tick-limit outcomes.
No audio effect is claimed by ignoring it in this control-only layer.

All 766 native ticks match the core's control fields on the host and on the
emulated 68030. Native core run `flowcore1789910145095272000` completed in
28.254 seconds, including 1..16-track and 128/256-position boundaries, order wrap,
ordered simultaneous globals, stable stop/limit outcomes and atomic invalid init.
`native-flow-core.json` retains each result; both native windows ended with all
Paula DMA channels off, launch script restored, no emulator process/private HDF
holder/socket and explicit release to AmiConnect.

All 37 host checks pass (43.734 seconds), including ASan/UBSan core comparison,
trace framing/count/stop corruption rejection and diagnostic anchor checks.
`core-build.json` records sources, compiler/runtime and binary hashes; native
cross-build uses the existing warning-as-error 68000-compatible flags. The
portable core and diagnostic are separate test binaries, not editor additions.

Observed reference behavior includes B01+D12 entering order 1 row 12, the reverse
command order entering row 0, D0A entering row 10, F00 followed by F06 remaining
stopped, EE2+D03 advancing the next fresh fetch to order 1 row 4 and E6 start state
persisting across patterns. Natural wrap/backward jump cases end on the test
budget; they are not falsely classified as completed songs.

Reproduce host checks with `make test`, cross-build with `make core-tests`, then
reserve the isolated emulator before running `tools/test_flow_emulator.py` and
`tools/test_flow_core_emulator.py`. The first script produces a new uniquely named
capture directory; the second compares the committed reference corpus. Both
restore the private launch hook and guard the emulator profile before control.

Limitations: this does not establish elapsed CIA-cycle/sample timing, all-effect
coverage, PCM rendering, audible parity, live mixed routing, physical AmiGUS or
real 68030 performance. No duration/render UI is wired to this core yet. Broader
flow combinations, one-pattern start modes, clock accumulation and the voice/mix
core remain subsequent work. No physical A1200/AmiGUS was controlled.
