# Exact small-delta arithmetic; cadence still FAILED

Elapsed advance/deadline use32-bit products/division when their numerator fits, otherwise unchanged64-bit split/borrow arithmetic. Exactcarry, frequency/regression/overflow poison and atomic errors preserved. No relaxed deadlines or new epoch. Applies to short service deltas and short future deadlines on68000 software division.

Four host sanitizer suites PASS25.651s: independent128-bit product oracle400000 advance/deadline comparisons across fast/fallback threshold boundaries, random carry/frequency/rate/currenttick/currentframe and overflow; nativePaula transport; PaulaSong; wavetable dispatch/scheduling. Existing partition-invariance/earliest-deadline/extreme-borrow/atomic-refusal cases pass. Oracle is host-only; no new native target added.

Verified152compiledsource digests/no generated; treeed64d9e51292dd31f95f12b737d38e1a9bde5ae9. PTExecPaulaTransportTest201616bytes SHA25604d4d6818e3e360ecbbaa0f45a140886871d99e467e5c22b4c77376d2b34f98f. Guard built NOTexecuted.

Fresh scoped native run1790852679044255000 RC20 aggregateFAILED. Cancel/late cases0/1PASS. Cadencecase2calls5/completions2/observed837frames: phase4ENTRY watchdog sees842-last575=267>256. Maxentrygap3946ticks at709379Hz (~5.563ms); maxprior successful totalcost3912ticks (~5.515ms). Last observation cost69ticks. Previous post-watchdog and uninstrumented failures remain separately FAILED. These few calls do not establish stable performance improvement or full24000-frame cadence; current/target counter logs show low32bits of priorarm only. No musicalboundary/output qualification.

Candidate-specific failure path closes bothalarms/EClock/audio/editor/master before27Fastalloczeroownedbytes. Exactcleanup and separate subsequent sharedlocked running/profile/bridge68030/all4DMAoff/run+launcherabsencePASS. Explicit RELEASE sent; nohold/ownership/nextwindow. NoWRITE/cache/output/Wait/frontend/listening/physical claims. No automatic retry/reset/resume/weaken/rebase. Next host review targets repeated full ownership/current checks and counter-deadline calculation in serialized service without removing stale/clock guards.
