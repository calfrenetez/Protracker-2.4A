# Allocated stem batch workspace

Batch report,16 measurements, working options and staging/file paths now use
bounded caller allocation. Native batch/WAV/mixer share the master Fast pool;
up to3 working blocks coexist. Legacy stack entry remains available.

Host ASan/UBSan file/stem tests PASS (2 tests,17.440s) and all4 WAV/11 stem
allocation failures PASS (1 test,2.916s). Pinned native build PASS. Separate
shared030 allocated-stem and failure checks rc0/PASS within90 seconds each,
with hashes in results. Final done markers preceded exact owned cleanup and
explicit AmiConnect release. No UI/startup/restart/physical operations.

Compiler per-function stack estimates: allocated entry84 bytes versus legacy3624,
shared helper116. Not whole-stack peaks or real-hardware performance. Small locals,
caller UI frames and runtime library allocations remain separate audit work.
Latest full105 suite remains802ebc7; affected suites were run for this change.
Unrelated dirty display changes preserved, no release/UI/live Studio/card claim.
