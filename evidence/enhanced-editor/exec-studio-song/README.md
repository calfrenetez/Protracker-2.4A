# Native Studio song memory acceptance (shared030 only)

Coordinated with AmiConnect; live shared guest/bridge/68030 guards and test.lock
passed. PTExecStudioSongTest completed with rc0, SONG SESSION and EXEC MEMORY
markers.18 allocations passed TypeOfMem Fast/not-Chip checks; zero owned bytes at
finish. Production pool budget refusal was exercised without Chip fallback.

Guest uses256-frame blocks for normal, lead-in and row-range songs plus allocation,
stop and acquisition-failure paths. Host retains full1/17/256 partition comparisons.
Pinned cross-build and host song test passed. Runner verified done, cleaned exact
run-owned files/launcher and released lock; shared window explicitly released.
Run: render-files-1790214782740356000. Exact binary hash in result.json.

These are tracked controller/sequence/mixer allocation and guest algorithm results,
not CRT/static memory, audio-device, physical A1200, CPU deadline or AmiGUS proof.
Editor UI/live output remains unwired. Unrelated display work was preserved.
