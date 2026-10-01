# Silent native four-voice output and cleanup PASS

Candidate tree8fda84b77b3d8d2709e17733064d83da0e1c5cee; all92 compiled source
hashes verified against that tree; zero generated sources. Output17148bytes SHA256
86a069bf765ae1b10ae0ac443e33f02d39acd1d98c72cb1437b92d7e38560e68.
Guard built, not executed. Host24 output/12 reservation sanitizer cases plus
routed voice regression PASS (three suites5.068s). Earlier invocation of voice
suite without PYTHONPATH failed import only; corrected invocation recorded here.

Run1790844055834014000 normal PASS in1.788924083s including independent cleanup.
Actual audio.device exclusively owns four channels/key, silent zero32byte Chip
buffer/volume0. Each start requires channel DMA off/master on before BeginIO,
then async keyed errorfree WRITE plus channel/master DMA enabled afterward.
Pending/failed activation remains uncertain; late start messages remain retained.
All four DMA enabled asserted; four PERVOL commands completed. Stop aborts once,
polls CheckIO/WaitIO, drains notifications/control completions and observes DMA
OFF before release. FREE/LOCK completion and all requests/ports/device closure,
all4DMAoff/privateportsabsence precede bufferfree. Initial DMAoff before staging
and launch, exact harness cleanup and subsequent independent locked running/
all4DMAoff/run+launcherabsence PASS. AmiConnect RELEASE acknowledged and forwarded to peers; no next window reserved.

Read-only DMACONR, no MMIO writes/custom IRQ/UI/lifecycle/retry/reset/physical.
Prior RC14 candidate remains FAILED; separate one-slot diagnostic proves its own
late notification only. This qualifies silent device activation/control/closure,
not frontend installation, first audible sample, punctual timing, listening,
endurance or physical hardware. Native OS-reported PAL clock3546895 used here.

Register activation semantics: [Amiga Hardware Manual](https://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node00E0.html).
Read-only DMACONR and DMA enable bits: [register summary](https://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node002F.html).
