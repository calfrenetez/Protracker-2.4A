# Bounded project/sample save workspace

Native PTG/MOD/sample WAV/raw/IFF paths now allocate file state/paths and4096-byte
verification block through shared bounded master pool. Descriptor verification
replaces stdio read-ahead; short reads/EINTR, exact EOF and existing destination
preservation are covered. MEMORY reports explicitly without marking project saved.

All107 host tests PASS190.413s. New save fixture also runs forced7-byte reads and
repeated EINTR, checks9000-byte and empty output, allocation refusal, unchanged
input and existing-file preservation. Pinned native build PASS. Shared030
PTExecSaveTest rc0 with both PASS markers within90s:3 explicit allocations verified
Fast/not Chip, zero owned bytes after cleanup. Exact result hash recorded.
Final done marker preceded exact owned run/launcher cleanup and AmiConnect release.

Compiler per-function stack estimates: allocated entry40, legacy7160, verify52
bytes; not whole-stack/physical performance. No UI/startup/restart/physical run.
Dirty display changes preserved and not editor-release qualified. Audit in
MEMORY_AUDIT.md records remaining import stdio, recent-list workspace and runtime
internals; no claim these are already budgeted. Live Studio/AmiGUS remains open.
