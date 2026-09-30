# Whole-song mixed numerical schedule qualification

30 September 2026. Supplied numerical frame timestamps only; no native clock,
timer, actual device/MMIO/DMA/audio, physical or full native PLAY acceptance.

ONE retained shared sequence now schedules primed startup and every positive
emitting interval at absolute tempo boundaries. Pre-start cache/conversion work
emits no output. Exact startup requires readiness; each later boundary uses the
strict interval gate without allocation/upload/command conversion. Manual APIs
refuse during the schedule. Overflow/regression and missed/unready boundaries
poison and cancel the schedule, request retained stops once, and preserve both
ownership tokens and union Fast-master pins until confirmed close. DONE is
terminal/idempotent. No catch-up, implicit preparation or recovery retries.

ASan/UBSan host regression PASS7.916s: ALL147 scenarios, modes0..48 at8/16/24bits.
Native executable intentionally qualifies ONLY33 new schedule scenarios,
modes38..48 at8/16/24bits. Earlier114 scenarios have current host regression and
historical native evidence; this run does not requalify them natively.
New coverage includes whole-song44.1/48kHz timing with unselected-track tempo,
primed startup/no early output, exact allocation/upload-free boundaries, overflow,
regression, unprimed/late startup, live missed/unready boundaries, partial/ready
startup cancellation, failed AmiGUS start, retired prepared caches, retained active
readers and uncertain barriers. Sampled monotonic clock binding remains unfinished.

Selected portable/native owner plus guard build PASS160 indexed sources,
zero generated sources,3 verified binaries. Exact source tree/hashes recorded.
ONE normal shared030 software fixture PASS23.687s,363 actual Fast allocations,
zero owned bytes; actual Chip allocation/release assertions included.
Bounds200s orchestration/140s runner/90s fixture; reservation<=240s.
Fresh AmiConnect no-conflict/no-hold and peer checks, standard locked live guards,
independent subsequent locked running/all4DMAoff/exact owned run+launcher absence
PASS. Explicit RELEASE sent after verified cleanup. No guest retry/reset/resume.
The earlier candidate banner correction/rebuild happened before ANY guest access;
this corrected candidate was the only actual guest run.

qualification.py expects its original build/dev/run-mixed-schedule-qualification.py
location. No binaries tracked. Historical evidence is retained in sibling folders.
