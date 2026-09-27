# Native EClock read ownership — 27 September 2026

Separate OS2+ timer.device owner with private port/request, local device base,
64-bit counter and reported frequency. Request NEVER submitted. Host fake Exec
ASan/UBSan test passed0.348s: version/port/request/device failures, reverse cleanup,
double-open/close, output assembly/alias/null/zero-frequency and closed-read checks.

Native build/main syntax passed;169 staged inputs/font and binary hash verified.
Binary384752bytes SHA256
07d698eebbc7f92fcc99956a655f137a55ee06ee168c25203cf37c0c196bb099.
Shared030 render-files-1790505155225622000 passed within90s RC0. Real emulator
timer.device read across3open/close epochs,101reads each; monotonic64-bit counts,
stable reported frequency, checked frame/deadline conversion and closed-read
refusal. Existing ownership regressions also pass.1182Fast allocations, zero
owned sample bytes/noChipfallback. Timer resources explicitly closed before
fixture completion; all4DMAoff/exactrun cleanup/independent absence verified.
Shared030/DevBench explicitly released afterward.

This is native clock access and resource-lifetime evidence. No timer request,
sleep/wakeup accuracy, playback cadence, card output or physical acceptance.
Existing editor display timer/layout and unrelated dirty work unchanged.
