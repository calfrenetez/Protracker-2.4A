# Studio control batches

Two targeted host tests and pinned native build PASS. Coordinated shared030
PTExecStudioTest PASS rc0+done;2 tracked Fast allocations, zero owned bytes at
completion. Exact owned cleanup and explicit AmiConnect release confirmed.

Control tests verify pitch/gain changes preserve phase, muted voices advance,
invalid second-channel step refuses the whole batch, oversized gains/masks and
inactive channels refuse, empty batch succeeds, and controls do not churn pins.
Existing true24/clipping/ownership checks remain. No device output or physical
performance qualification. Native editor scheduling/transport remain unwired.
