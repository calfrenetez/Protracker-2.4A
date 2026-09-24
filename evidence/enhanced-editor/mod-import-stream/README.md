# Bounded uncompressed MOD import

Host sanitizer tests pass exact original MOD roundtrip, inactive order entries,
loops, interrupted/short reads, every read/allocation failure and final-close
callback failure preserving the old document, budget/size refusal and routing.
Document regressions pass. Source must remain stable during synchronous reads.
Reader decoder targets unpublished staging; only the document wrapper commits.

Shared030 run1790235169106443000 rc0/PASS with fresh AmiConnect clearance,
shared lock and live guards: 117 Fast allocations, zero owned bytes, budget
refusal without Chip fallback. Completion confirmed, staging clean, exact owned
cleanup passed, explicit release sent to AmiConnect and AmiWBMonitor. No physical,
card, audio, lifecycle or configuration operations. No editor UI acceptance.

The editor and converter use this path for recognised uncompressed MOD input.
Host converter tests pass MOD/project conversion with 538110-byte peak under a
550000-byte ceiling, final zero; existing destinations remain unchanged.
Enhanced project, PP20 and module-donor preview input remain whole-buffer paths.
Native converter runtime is not covered by the MOD import core fixture.

Full pinned native build passes, including updated converter. The runtime-tested
MOD fixture hash is unchanged in the final build.
