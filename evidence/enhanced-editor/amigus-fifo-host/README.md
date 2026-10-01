# Disabled FIFO preparation — host only

AmiGUSTest0.6:23760 bytes, SHA-256
6c87f24a1d9e3d4395c39a884d4c203980383bf7641be3c9ec3e6ae9dc8bd5f5.
Thirteen host checks pass, covering physical qualification/driver restoration,
model FIFO/packing/session/reset/reservation/interrupt ownership guards.
The pinned native build and inspection show one MOVE.L to the adjacent data
ports. The own-memory native fixture is built but has not executed yet.

The diagnostic first requires idle status, exact Mini firmware7ea663e7,
exclusive PCM access and no installed interrupt. It permits only three zero
long stores, expects pending-word counts2/4/6, then requires a confirmed reset.
Failure/refusal to reset retains the Task/card/access/base and restoration
obligation; no repeat writes, unsafe release or AHI reload. Confirmed reset
allows ordinary release even when the count test failed, preserving that failure.

Shared030 runtime qualification and separate physical test are pending.
No FIFO capacity, physical word ordering, IRQ teardown, output or listening
acceptance. Installed files and settings remain unchanged.
