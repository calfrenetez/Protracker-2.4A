# FAILED cadence — POST watchdog identified

Transport retains refusal phase and observed/last service frames before cleanup; private alarms retain last actual sampled counter/target. No added clock I/O. Counters are diagnostic only, not scheduling input. Last observations survive release; a successful begin/open resets corresponding diagnostics.

Four host ASan/UBSan suites PASS21.788s: diagnostic retention through delayed timer close, PERIODIC_ARM injected-cost refusal, existing ownership/exact start/clock failure cases. Initial alarm-close reset erased diagnostics, caught by test and corrected; final native candidate includes retained transport observations even if core initiates cleanup before wrapper snapshots failure.

Verified152compiledsource digests, no generated; tree014dea7a30fe4bce4146825a28ed8f70c2bc1c19. PTExecPaulaTransportTest201276bytes SHA25683d0f9f18514a0fe829aabaa5a21e80f8c2993632685833445afddad87265ebe. Guard built NOTexecuted.

Fresh coordinated run1790852344357003000 RC20 aggregateFAILED, original failure preserved. Cases0cancel/1deliberate late refusalPASS. Case2calls3/completions1/observed526frames; phase9=POST watchdog: service_frames810-last_frames548=262>256. Last actual observation minus service entry4203ticks at709379Hz (~5.925ms); max prior successful total cost3301ticks (~4.653ms). Entry gap3335ticks (~4.701ms). Recorded periodic counters are low32bits; they belong to the previous arm and are NOT the refusing operation. Stage identifies post-service time cost; no assertion about exclusive rootcause or physical performance.24,000-frame cadence not achieved.

Candidate-specific safe failure path checks bothalarmports/EClock/audioreservation/active/DMAclear, boundedstop/detach/dispose beforeFastfree;27Fastallocations endzeroownedbytes. Exactcleanup and subsequent independent sharedlocked running/profile/bridge68030/all4DMAoff/run+launcherabsencePASS. Explicit RELEASE acknowledged and forwardedpeers. No WRITE/cache/output, no cadence/frontend/Wait/listening/physical qualification. No automatic retry, reset, resume, rebase or deadline relaxation.
