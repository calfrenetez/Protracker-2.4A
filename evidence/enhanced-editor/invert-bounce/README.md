# EFx bounce transaction — 26 September 2026

Host sanitizer tests pass ordinary and opt-in EFx bounce, exact24 PCM,
all planning/filling allocation failures, source preservation, budget and
cancellation rollback, retained redo, undo/redo and enhanced-project save/load.
Ordinary WAV/stem allocation-failure regression also passes. The full156-test
suite passed immediately before this change; only relevant regressions repeated.

Canonical focused native build (recorded compiler/runtime flags and source hashes)
passed. Shared030 run1790462258371217000 PASS:137 production Fast allocations,
all not-Chip, zero owned bytes on completion, explicit budget refusal without
Chip fallback. Completion/all4DMAoff/exact cleanup passed; window released.
No physical timing/audio or editor-UI acceptance. This opt-in API is not yet
wired to native editor controls or queued Studio.
