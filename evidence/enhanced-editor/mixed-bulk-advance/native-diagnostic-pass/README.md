# Bulk phase advancement: native diagnostic PASS

1 October 2026. Indexed source tree6217ce5831ebd2d6e321ada6afb1728108d6cf9d:
163 source hashes verified against indexed tree and candidate, zero generated,
4 binaries verified. PTExecMixedOwnerTest237396bytes SHA256
84ebcd0aedde31611cddbc07f7dba6f116f52105c1773ba06d5211f8a13d515b.
One separately coordinated shared030 run1790834570494139000 PASS27.440s overall,
90s fixture/140runner/200overall bounds, <=240s reserved. Flushed phase markers
confirm all40 frame-reader trajectories,32random16voice partitions,256wide scalar
trajectories, then6complete mixed8/16/24-bit running-cost cases. Host4096random
partitions+256wide+40reader and5relatedASan/UBSan suites remain separate evidence.

66actual Fast allocations/zeroownedbytes; nativeChip released; all injected readers,
master pins, cache leases and private unsubmitted EClock counter/device/request
closed. Normal done/rc0 and exact run+launcher cleanup verified, then independent
subsequent locked bridge/68030/running/all4DMAoff/pathabsence PASS. ExplicitRELEASE
acknowledged AmiConnect and forwarded peers. PhysicalOFF/unprobed/untouched.

858actualEClock observations at709379Hz, all outliers retained in native.log and
observations.json: eachvoicecount384prep/39service/6boundary.2voiceprep0.954..31.563ms,
service1.166..1.720ms,boundary4.137..4.684ms;16voiceprep0.956..36.604ms,
service2.339..2.950ms,boundary6.358..7.240ms. Previous16voice service18..38ms is
recorded in mixed-running-cost, but this is an instrumented emulator observation,
not a controlled hardware benchmark.128frames48k=2.667ms:16voice still exceeds
that span in some calls; boundaries also exceed it. No actual-time feasibility,
real output/MMIO/DMA/IRQ/device/audio or physical/listening acceptance inferred.
Strict timing/ownership/cache policies unchanged.

Original run1790811697827119000 remains FAILED/incomplete at90s, unknown cause.
It was not retried or promoted. Explicitly approved recovery is separately retained
under recovery-1790834032806376000; this new binary adds flushed diagnostic markers
without changing test workloads or budgets. A new pass does not explain the failure.
