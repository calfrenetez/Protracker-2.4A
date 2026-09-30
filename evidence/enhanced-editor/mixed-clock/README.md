# Mixed sampled-counter binding qualification

30 September 2026. Injected counters and software/native allocation fixture only.
No actual native clock/timer/output/MMIO/DMA/audio, physical or full PLAY acceptance.

Clocked begin takes one immutable serialized monotonic counter reader and a frame
delay from its first observation. The checked elapsed-clock converter retains
fractional carry without per-poll rounding drift. Service advances the SAME
whole-song mixed schedule; deadline reports the first counter tick at/after its
frame threshold without clock I/O or allocation. NULL outputs refuse without
reads. Numerical/manual APIs refuse once bound. Counter read/frequency/regression/
overflow failures poison/cancel/stop once and retain unconfirmed readers and union
Fast-master pins until confirmed close. Low-frequency counters skipping exact
frame boundaries refuse DEADLINE rather than catch up. DONE is terminal, leaves
outputs unchanged and never rereads the counter. No rebase or automatic recovery.

Full ASan/UBSan host regression PASS7.798s:168 modes0..55 at8/16/24bits.
Native intentionally qualifies ONLY21 new modes49..55 at8/16/24bits: fractional
half-frame observations, whole-song44.1/48kHz global tempo and exact boundaries,
initial reader failure, changed frequency, regressed counter, active reader failure
with uncertain stops, unrepresentable deadline and skipped exact startup frame.
Earlier147 cases have current host regression and historical native evidence;
this native run does not requalify them. No allocation/upload/command conversion
at exact output boundaries; readers/pins survive failed cleanup barriers.

Selected portable/native owner+guard build PASS160 indexed sources/zero generated/
3verified binaries; exact source tree/hashes recorded. ONE normal shared030 fixture
PASS14.611s;231 actual Fast allocations/zero owned bytes, actual Chip allocation/
release assertions. Fresh AmiConnect no-conflict/no-hold and idle peers, standard
locked live process/profile/68030/running/bridge guards, exact owned cleanup and
independent subsequent locked running/all4DMAoff/run+launcher absence PASS.
Explicit RELEASE sent. Bounds200s orchestration/140s runner/90s fixture with
reservation<=240s. No retry/reset/resume/physical/screenshot/lifecycle operations.

qualification.py expects original build/dev/run-mixed-clock-qualification.py
location. No binaries tracked. Native event-loop, actual timer and device output
integration, repeat/segment/range support and physical performance remain open.
