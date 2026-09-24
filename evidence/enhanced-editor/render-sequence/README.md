# Incremental renderer sequence with Studio integration fixture

One allocator-owned session reuses audited renderer preflight/measurement,
timeline, pitch, ranges and command plans. It exposes the preceding audio interval,
requires successful bounded audio consumption before completion, and refuses
out-of-order calls. Internal tick/command failure poisons the session. Project
storage remains borrowed/immutable; closing the sequence does not close Studio.

The integration fixture compares a complete Studio song against reference true24
output at block sizes1/17/256, with tempo changes, pattern delay, volume slides,
new notes, lead-in and row-range pre-roll. Allocation, unsupported-effect and
protocol refusal cases are included. PTSequenceTest is cross-built for Amiga.

No emulator/physical execution, editor UI or real-time output qualification here.
Native build includes unrelated display work and is not a release candidate.
The caller-driven song path still needs editor lifecycle and bounded output queues;
AmiGUS transport and physical acceptance remain open.
