# Studio independent segment triggers

Three targeted host tests and pinned native build PASS. Sole coordinated
PTExecStudioTest shared030 run rc0+behavior/EXEC PASS+done,4 Fast allocations,
zero owned bytes. Exact owned cleanup and explicit AmiConnect release confirmed.

Segment fixture compares three blocks with the existing voice reference using
half-frame linear interpolation, independent/disjoint initial+repeat ranges,
invalid replacement preserving both pins, successful retrigger dropping pending
handoff, and complete cleanup. It also runs all earlier handoff/control tests.
No audio transport, real-time deadline or physical performance claim.
