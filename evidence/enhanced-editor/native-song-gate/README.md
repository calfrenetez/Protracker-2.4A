# Native real-clock refusal — 27 September 2026

Fixture-only integration: production clocked song service with native EClock,
absolute alarm and independent watchdog, but injected voice callbacks. Tiny24-bit
master/cache leases fully prepared before200ms scheduled startup. Diagnostic
compares the actual service reader timestamp to frame9600. Late results must be
DEADLINE, preserve caller output, issue no start/control/restore and release leases.
An exactly on-time observation may proceed only through fake callbacks then close.
Second case explicitly adds Delay(1) to guarantee a late-service regression.

Native build/main syntax PASS;170staged inputs/font and binary verified.
Binary401268bytes SHA256
baf2d71d8678dbcd524e46f14eac55361db9e80f4faced8ff49ab1695f7a234e.
Shared030 render-files-1790506238580825000 PASS within90s RC0. Natural wake reached
frame9656, delayed case10778; both returnedDEADLINE(14), zero start callbacks,
released all prepared leases. All watchdog/alarm/clock resources closed before
completion;1195Fast sample allocations/zeroowned/noChipfallback. All4DMAoff,
exactrun/launcher cleanup and independent absence verified; explicitly released.

Signal diagnostic in same candidate:16/16beyond one48kHz frame,193..9904late ticks
at709379Hz. Refusal/lifetime acceptance only, not usable real-time playback.
No tolerance relaxed, no native/card audio or physical/lifecycle/config operations.
The actual time between service reader sampling and synchronous callbacks remains
unqualified; source guard/preparation/dispatch cost needs characterization before
choosing native timing integration. Unrelated dirty display work excluded.
