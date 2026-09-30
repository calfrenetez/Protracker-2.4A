# Coordinated mixed prepared commit qualification

30 September 2026. Software fixture only: no real AmiGUS/MMIO/Paula DMA,
audio, clock, physical or full native PLAY acceptance.

Private ready-batch commit validates BOTH engines and candidate/live cache leases
before output; cached commands emit in original global order. Commit makes no
allocation/upload/command conversion. Duplicate trigger leases transfer separately.
Late cache/reader refusal cancels candidates without output. Partial callback
failure poisons both engines, stops retained readers once (no immediate retry of
a failed stop), retaining masters/tokens until both reader/barrier drains confirm.
Whole-song mixed lookahead/scheduling remains unfinished.

Four ASan/UBSan host regression modules PASS27.443s. Native build selected portable
and Exec mixed-owner targets plus guard:160 indexed sources,zero generated,
3verified binaries. Source tree and binary hashes in source-tree.txt/core-build.json.
Host launcher initially failed on interpreter import before guest access; the shared
harness Python ran the single actual guest fixture. No guest retries/recovery.

Shared030 fixture PASS37.484s;732 actual Fast allocations, zero owned bytes,
actual Chip allocation/release assertions. Overall200s/runner140s/fixture90s bounds.
Fresh AmiConnect no-hold/no-conflict, peer status, standard locked live guards,
exact candidate hashes before run. Independent subsequent locked running/all4DMAoff/
owned run+launcher absence PASS. Explicit RELEASE sent after confirmed cleanup.
Qualification source copied from build/dev/run-mixed-commit-qualification.py;
its relative root calculation expects that original location. No binaries tracked.
