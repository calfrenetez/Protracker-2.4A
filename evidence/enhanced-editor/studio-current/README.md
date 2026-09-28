# Current Studio software fixtures

Five exact full-build fixtures passed shared030 in 61.376 seconds: master-pinned
Studio mixing, sampler promotion, PCM session, packed FIFO chain and Studio song.
Source tree and exact executable hashes are recorded in planned.json; this is the
147-target build already verified against 350 source hashes and 207 host cases.

Checks cover direct 24-bit mixing, retained sample versions and segment handoffs,
edit/undo/eviction pins, explicit tail flush and drain acknowledgement, failed
reset lease retention, packed-byte equivalence across odd partitions, and song
PCM equivalence with the reference renderer across tempo, delay and pre-roll.
Fast allocations respectively 9, 11, 7, 4 and 21; all returned zero owned bytes.
All fixtures returned zero. Each was followed by a separate locked check of
running guest, all four Paula DMA channels off and exact run/launcher absence.
The coordinated window was explicitly released to AmiConnect after completion.

These are CPU and injected FIFO software checks. They do not establish real
AmiGUS access, MMIO, native device output, listening or physical acceptance.
The physical Amiga remained off.
