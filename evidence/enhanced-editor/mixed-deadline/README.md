# Mixed numerical interval deadline gate qualification

30 September 2026. Injected numerical software/native allocation fixture only.
No actual clock/timer/device/MMIO/DMA/audio, physical or full native PLAY acceptance.

Clock arm requires a fresh full positive emitting interval of the SAME shared
sequence. Service takes supplied absolute FRAME timestamps, advances live elapsed
debt <=256 plus one forecast/cache step before the boundary. At exact deadline BOTH
routes must already be ready and final debt <=256; commit does no preparation,
allocation/upload/command conversion. No early starts. Manual next/consume/prefetch/
complete/prepare refuse while armed. Regression/overflow poisons CLOCK; late/unready/
excess debt poisons DEADLINE, cancels sequence/candidates and attempts retained stops
once, preserving unconfirmed readers/pins until close. No catch-up/retry.
Whole-song scheduling and sampled monotonic clock binding remain unfinished.

Expanded ASan/UBSan fixture PASS7.900s:38 modes at8/16/24bits. New cases cover
44.1/48kHz exact boundaries, constant pre-boundary services and no early output,
manual/double/zero-frame refusal, write/allocation-free exact commit, regression,
start overflow, late/unready deadline, ready but excess live debt, armed close,
prepared-cache retirement before commit and uncertain stop retention without
repeat fault cleanup. Earlier shared sequence/master/cache/commit cases retained.

Selected portable/native owner targets+guard PASS160 indexed sources,zero generated,
3verified binaries; hashes/source tree recorded. Shared030 ONE normal fixture
PASS71.515s:1227 actual Fast allocations, zero owned bytes, actual Chip allocation/
release assertions. Bounds200s overall/140s runner/90s fixture, reservation<=240s.
Fresh AmiConnect no-conflict/no-hold plus idle peers; standard locked liveguards,
independent subsequent locked running/all4DMAoff/exact owned run+launcher absence
PASS. Explicit RELEASE sent after confirmed cleanup. No guest retry/reset/resume.
Qualification script expects original build/dev/run-mixed-deadline-qualification.py
location. No binaries tracked. Numerical checks do not prove actual timely output.
