# Bounded PP20 packed input

Host ASan/UBSan PASS: independent original decoder and Python bit-writer parity
for all skip counts0..32, literal lengths1..64, match lengths/offset forms;
mutated/truncated rejection, bounded256-byte codec reads, every read failure,
all7 staging/scratch allocation failures, finish failure, budgets and exact MOD
roundtrip. Short/interrupted file reads and document regressions pass.

Converter large packed input produces exact MOD at peak664104 allocator bytes
under700000, final zero. Native editor full/donor and converter paths now avoid
full packed input. Full unpacked scratch remains necessary and budgeted. Stable
source required. PX20/nested/enhanced payloads still unsupported.

Full pinned native build PASS. Coordinated shared030 converter run
1790238037622298000 rc0/PASS: exact2172-byte MOD export and packed input preserved.
Done and byte checks preceded exact owned cleanup; explicit release to AmiConnect.
No lifecycle/config/audio/card/physical changes. This proves the converter path,
not donor/editor UI or physical acceptance.
