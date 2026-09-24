# Bounded enhanced-project file import

Host ASan/UBSan PASS: exact mixed v1 master roundtrip, all read/allocation/finish
failure rollback, file limit/budget rejection, short/interrupted reads. Document
regressions pass. Native editor and converter full-document load select the
project reader for enhanced magic. Adapter has a fixed 4096-byte read cache and
checks length/EOF/close before document commit; input must remain stable.

Converter large-sample enhanced-project-to-MOD conversion passes below a
550000-byte allocator ceiling, peak538110 and final zero, exact original MOD.
This excludes a complete encoded input beside decoded masters. PP20 remains on
the existing packed/unpacked-buffer path. No enhanced import runtime, editor UI
or physical acceptance is claimed by these host checks.

Full pinned native build PASS. Fresh coordinated shared030 converter run
1790236934272234000 rc0/PASS: exact26416-byte mixed-master roundtrip and unchanged
source. Completion and byte comparison confirmed before exact owned cleanup;
explicit release sent to AmiConnect and AmiWBMonitor. This executes production
Fast-memory allocator paths but does not measure physical memory/performance.
No lifecycle/config/audio/card/physical changes. Editor UI acceptance separate.
