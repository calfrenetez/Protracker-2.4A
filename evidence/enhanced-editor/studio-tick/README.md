# Studio interval partitioning

Three targeted host tests PASS (Studio mixer, interval reader, frame clock),
with ASan/UBSan. Pinned native build PASS, including PTStudioTickTest. No emulator
or physical run for this milestone.

Three hundred ticks alternate127/131/241 BPM;1/17/256-frame block partitions yield
identical audio hash cbdaeaff and accumulated Q32 frame totals. Checks cover
undrained-interval refusal, invalid BPM/read preservation, exact frame-budget
exhaustion, allocation failure and independent reader/mixer cleanup.

The interval reader borrows the mixer and does not dispatch tracker effects,
transport PCM, measure deadlines or prove real-time capacity. These remain open.
