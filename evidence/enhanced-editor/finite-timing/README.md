# Finite timing diagnostic: emulator qualification

The dedicated no-argument `PTExecWavetableTimingTest` is 259392 bytes, SHA256
`09a401411e66710d24828769960a3a784127d365a439c87027340d74a622b372`.
It uses the same clock, alarm, watchdog, song gate, fake voice cost and snapshot
fixtures as the full editor test. Its final marker follows memory teardown.

Shared core run `20260927T120335293381Z` completed under a 45-second command
limit with exit code 0, 29 Fast allocations, zero owned bytes and no Chip fallback.
All timer requests, watchdogs and sample leases closed. All four Paula DMA channels
were off. The exact two run-owned files and directory were removed after archiving;
independent directory absence was verified and the window explicitly released.
The existing shared physical promotion function independently accepts this exact
result and artifact hash. No shared installed configuration or scripts were changed.

Readiness took 745–755 EClock ticks; one/16-voice simulated starts took
4407/27364 ticks at 709379 Hz. These are emulator observations, not physical or
realtime acceptance. No audio, hardware voice bus or AmiGUS operation occurred.

Host wrapper guards: 2 tests passed. Native build and main syntax checks passed;
all 174 input/font hashes matched the staged sources. The default full editor
fixture remains available; this diagnostic only selects its finite timing subset.


## Physical availability blocker

On 27 September 2026 at 12:11:18 UTC the single read-only request to configured
`http://192.168.0.156:8080/health` timed out with a five-second limit. No physical
target selection, upload, execution, timer request or RAM staging occurred.
The shared emulator connection was independently verified unchanged. The reserved
physical/DevBench window was explicitly released to AmiConnect, without a recovery
hold. There was no retry or interface/configuration change. Current A1200 power,
interface and address need owner confirmation before a new coordinated attempt.

The physical caller's eight combined host tests pass, including promotion refusal
before target selection and emulator restoration after connection, execution or
cleanup failure. The caller has not executed on hardware; physical acceptance is
still open. Emulator software qualification above remains valid.
