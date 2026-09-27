# Explicit native callback ABI bridge

The pinned SDK callback typedef specifies a0 but its nearby comment says d1.
Under our pinned GCC flags, a C callback written with that typedef was observed
reading its argument from the stack. This was found in disassembly before launch.
An explicit assembly entry now preserves d1..d7/a0..a6, pushes a0 for ordinary
C dispatch, and returns the normalized handled result in d0 with RTS.

The independent assembly caller supplies a0, poisons d1, and verifies context,
handled/unhandled/invalid results, null/unarmed refusal and preservation of d1,
a0 and a1 despite a deliberately clobbering C handler. Disassembly checks the
full save/restore sequence. Existing private Find/Reserve/Free vector and resource
ownership tests also pass. No interrupt is installed or raised; this is ordinary
task-context execution and does not qualify actual interrupt safety or removal.

Candidate 21804 bytes, SHA256
36c400a7b2c94233bb1da75a9e5a8a0d219bee3b8d0b48a7d51dda913794f260.
All 11 source/header hashes match the staged index. Shared030 run
render-files-1790547885559715000 passed within90seconds, RC0, one Fast allocation,
zero owned bytes/no Chip fallback, all four DMA channels off. The harness reported
cleanup, but independent inspection found two empty run directories. Under fresh
shared lock/live identity/running/DMA guards, only those exact empty directories
were removed with rmdir; independent run/launcher absence confirmed and the
window explicitly released. cleanup-independent.json records that extra check.
Physical hardware, actual library callbacks, MMIO, audio and real interrupts
remain untested. The physical Amiga remains switched off and was not probed.

Build: tools/build_amigus_reservation.py --abi with pinned AMIGA_CC.
Run: coordinated shared harness --studio-memory native-abi.
