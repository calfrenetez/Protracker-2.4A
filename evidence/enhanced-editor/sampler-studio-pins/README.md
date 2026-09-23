# Sampler master pins and Studio provider

Sampler regression and new lifecycle test PASS with ASan/UBSan. Native build
PASS including PTSamplerStudioTest. No emulator or physical execution this turn.

The integration test covers document-backed true24 promotion, budget/allocation
refusal, stale-generation rejection, unchanged old playback across edit/undo/redo
and journal eviction, simultaneous old/new voices, then playback after release
of document/current/journal ownership while sampler/context remain alive. Final
close releases every pin and all sampler bytes; original source stays unchanged.

Native editor does not yet use this provider. Close sessions before normal
editor reset; never reinitialize/free sampler or allocator context with live pins.
Scheduler, transport and physical performance acceptance remain separate.
