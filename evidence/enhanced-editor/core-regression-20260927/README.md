# Broad committed-source regression,27September2026

Native build:139executables from immutable exported commit1ac4856; all288listed
source hashes and139binary hashes/sizes verified. Full manifest is retained in
../paula-regression-20260927/full-native-build.json. No unrelated display work used.

Host suite:200tests attempted in444.347seconds.198passed. Two Git-dependent tests
could not export sources from inside the nested archive (Git pathspec errors);
no product assertion failed. Both were rerun with their existing source-export
helpers at the actual repository root: editor_invert_reference PASS8.196seconds,
editor_wavetable PASS29.859seconds. These helpers use committed/index sources,
respectively; source/test trees were unchanged from1ac4856. Thus all200distinct
host tests have passing execution, with original setup errors retained here.
This is a combined result, not a claim the initial full invocation exited0.

Shared030 core-qualification-20260927T223729377173Z ran all14registered
noninteractive core cases, each RC0 and exact manifest hash. Per-case45seconds,
aggregate300seconds. Covers sampler/slots,8/16/24PCM,RAW/IFF,filter,slices,
pattern/record/channel lifecycles, MIDI byte logic, file publication and EFx core.
MIDI tests are injected/byte-level, not actual MIDI devices. No audio/AmiGUS or
physical operations. All4DMAoff before exact binary/log-only cleanup, independent
run/launcher absence verified and AmiConnect explicitly released.

The shared runner reads parent checkout Git status from the archive; its `commit`
field names29fc9d9 and records unrelated dirty display files. `source_export_commit`
and exact candidate hashes identify the actual immutable1ac4856inputs. The
process-local profile changed no installed project or shared configuration.
Source build success and these14executions do not qualify every139binary or
native Studio/AmiGUS output, physical timing or sound.
