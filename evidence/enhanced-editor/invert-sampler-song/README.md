# Sampler private EFx producer binding

2026-09-26 UTC. Host sanitizer checks for ordinary sampler-song and explicit
private EFx sampler-song pass (2 tests,10.327s). The new binding verifies exact
first audio after stop/edit/restart and undo/redo, refuses unexpected generation
changes and replacement of each samples/events/orders/count field, and cleans up
all open allocation failures, budget/unsupported-precision refusal and normal end.
Master PCM is unchanged by playback. Calling stop before owner disposal permits
later harmless pull/close without consulting released owners.

Shared030 run render-files-1790465625419960000 passes RC0 within120 seconds. Native
production allocator reports93 Fast/not-Chip allocations, zero owned bytes and
budget refusal without Chip fallback. Exact compiler/runtime/input/binary hashes
are recorded in native-build.json; canonical m68000/nix20 build uses -fbbb=-.
All4 audio DMA channels off; exact run/launch paths cleaned and verified absent;
window explicitly released to AmiConnect and Scott. No lifecycle/config/device
or physical operation. This is core/sampler acceptance, not native editor PLAY,
AmiGUS transport, audible output or hardware performance qualification.
