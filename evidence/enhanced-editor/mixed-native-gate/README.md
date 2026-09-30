# Native mixed startup clock/alarm refusal qualification

30 September 2026. Actual emulator timer.device EClock observations and
UNIT_WAITECLOCK alarm/watchdog requests; voice and bus callbacks remain injected.
No actual Paula/AmiGUS output, DMA/audio, physical or successful playback acceptance.

The existing native counter/alarm adapters remain unchanged. The new fixture
binds their real monotonic reader to the SAME prepared mixed owner, primes both
routes before a one-second frame delay, converts the threshold to an absolute
counter alarm, waits on alarm/watchdog signals and resamples through production
clocked_service. Three natural-wakeup cases and three explicitly delayed Delay(1)
cases at8/16/24bit precision (44.1kHz for16,48kHz otherwise) all returned DEADLINE.
BOTH route start counts stayed zero in ALL6cases. This proves refusal/cleanup,
not timely dispatch or audio playback. Reported later_frame is an enclosing/later
diagnostic observation, NOT the exact clock sample used by dispatch. Frequency
709379 ticks/second; observed natural later_frame values48201/44286/48199 versus
starts48000/44100/48000. Explicit-delay values49367/45998/49731. These observations
include caller/diagnostic latency and are not precise wakeup-performance measures.

Failed combined quiescence barriers retain BOTH ownership tokens and union masters;
confirmed close releases them before counter/alarm disposal. Completed startup
alarm reply collected; future watchdog cancelled and its reply collected before
ports/requests/signals are released. Closed resource fields checked; native budget
fixture reports66actualFastallocations and zero owned bytes, actual Chip allocation/
release assertions included. No retry/reset/resume or force release.

Full ASan/UBSan host regression PASS7.666s:168 injected-counter/software scenarios.
Native scope ONLY6 new modes56..57 at8/16/24; historical native and current host
results remain separately attributed. Selected owner+guard build PASS162 indexed
sources/zero generated/3verified binaries, exact source tree/hashes recorded.
ONE normal shared030 run PASS10.289s for refusal/cleanup. Fresh AmiConnect scope
coordination and no-conflict/no-hold, idle peers, standard locked live guards,
exact run cleanup and independent subsequent locked running/all4DMAoff/owned-run+
launcher absence PASS. Explicit RELEASE sent. Bounds200s orchestration/140s runner/
90s fixture, reservation<=240s. No system-clock setting, CIA/vector takeover,
physical probes, screenshots or lifecycle changes. qualification.py expects its
original build/dev/run-mixed-native-gate-qualification.py location. No binaries tracked.

Native PLAY event-loop/device output and achievable timing policy remain open.
Continue host analysis and mixed guarded-dispatch cost observations before making
any timing-policy change; do not relax strict refusal or infer output acceptance.
