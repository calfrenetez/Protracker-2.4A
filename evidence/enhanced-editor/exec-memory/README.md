# Exec-backed shared030 memory acceptance

Production master_memory.h now backs native wrappers around existing bounce,
allocated-stem and all export-failure fixtures. Each explicit allocation is
verified with Exec TypeOfMem as Fast and not Chip; each fixture ends with zero
owned bytes. Bounce69 + stems54 + failure61 =184 allocations. Current queried
pool limits/reserves are in logs. Budget refusal uses a temporary zero ceiling,
not exhaustion of shared guest RAM; deterministic per-stage failures remain.

Pinned native build PASS. Host mocked-Exec allocator test PASS0.309s (ASan/UBSan).
All3 native wrappers returned0 with fixture and EXEC MEMORY PASS within separate
90-second windows. Result files record binaries; wrapper-inputs records additional
included-source hashes. Final done markers preceded exact owned cleanup and
explicit AmiConnect release. No UI/startup/restart/physical operations.

No production code changed. Latest full105 host suite remains802ebc7; this adds
native-only execution coverage. Does not instrument hidden C-library allocations,
prove physical fragmentation/performance, qualify fixture stack buffers as Fast,
or qualify editor UI/live Studio/AmiGUS. Dirty display work preserved.
