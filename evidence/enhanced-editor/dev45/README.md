# dev45: native tremolo evidence

Separate PTVolumeTraceTest adds stored volume and per-channel tremolo command,
phase, full wave control and vibrato phase to the existing 52-byte trace prefix.
The existing replay ABI already captures every final output volume before mute
gating at bytes32..35. No extra write hooks were needed. An initial build assertion
expected six raw write anchors, but only the shared wrapper has a write after
prepare_replay; the build stopped before any new diagnostic launch. The generator
was simplified to append state only. Shipping editor/render binaries are unchanged.

76-byte layout: existing52; four stored-volume BE words52..59; four groups of
command/phase/wavecontrol/vibratophase at60..75. Native baseline prefix must match
dev38 exactly. Eight synthetic fixtures are each captured twice. An independent
Python model checks every output, stored volume and phase/control field. It
preserves the native ramp magnitude dependency on vibrato phase, even though
tremolo phase selects the addition/subtraction sign.

Renderer 7xy/E7x remains unsupported. No physical tests performed.

Validation: existing 55 tests passed in95.780 seconds; the new native-trace checker
passed in0.005 seconds (56 total). Native run tremolo1789935441990802000 captured
17 traces in 44.18 seconds, with 186 fixture ticks per pass
and exact repeat equality. Baseline first52 matches dev38. Fresh guarded release
at20:18:50 UTC verified no process/HDF/socket and original launcher restoration.
AmiConnect received explicit release for its next requested window.
