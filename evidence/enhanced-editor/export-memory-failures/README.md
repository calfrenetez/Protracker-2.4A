# Export allocation-failure coverage

Host ASan/UBSan test passed (1 test,2.747s). Pinned native build passed and the
same PTRenderMemoryTest passed shared030 execution within90 seconds, rc0.
Binary hash is in result.json. Shared identity/lock guards applied; final done
marker preceded exact run/launcher cleanup and explicit AmiConnect release.

Every workspace allocation is refused in turn:3 WAV stages and8 stages for two
stems. The last stem failures occur after the first stem has been fully written
and verified. Every failure preserves project/master PCM and report bytes,
returns MEMORY, leaves no live allocations and removes all32 possible owned
staging paths. Host additionally asserts the entire output directory is empty.

No production behavior change was necessary. This test-only milestone does not
rerun the full suite (latest105 pass remains802ebc7), qualify editor UI, physical
Fast-memory fragmentation, actual storage failure or live Studio/AmiGUS output.
No UI/startup/restart/physical operation; unrelated dirty display work preserved.
