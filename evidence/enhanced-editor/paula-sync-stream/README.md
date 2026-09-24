# Bounded Paula sync workspace

Live sync no longer allocates a second complete encoded MOD. Its comparison
workspace contains only the header/pattern prefix; streamed sample/header bytes
are checked against the immutable original export. Instrument working-set
changes still require stopped cache rebuild. No edited rows publish until the
whole preparation succeeds. DMA/EF-mutated bytes are not used as the baseline.

Five host sanitizer suites pass. The new test compares bounded preparation with
the previous full-export compatibility check for accepted note/title edits and
rejected instrument/header/first/last-sample edits. A 131074-byte sample payload
uses only a 2108-byte row workspace; output guards, split sink blocks, truncated
capacity, alias rejection, unsupported precision and unchanged masters pass.

The native regression additionally asserts exact replay pool ownership:
one full immutable export plus two prefixes and three allocation headers.
Native/emulator results are recorded alongside this report when complete.
Physical audio, timing and editor UI acceptance remain separate.

Final native build passed. Coordinated shared030 run1790240773200318000
passed all five executables (rc0). The native exact-memory assertion passed,
as did live edits, changed-master/instrument refusal, cache reuse, audition,
allocation-pressure recovery and shutdown. All four DMA channels off before
exact owned cleanup and explicit release. CIAA fallback NOT RUN because
Workbench owns timer B. No physical operations or release-package update.
