# Prepared ordinary command bank — 27 September 2026

Each preparation step validates/converts one immutable trigger, control or stop
command. The private exact-batch/rate/format readiness check allows ordinary
commit to reuse those values, with zero trigger/control conversions at commit.
The entire live channel/source/held-state transition is still validated before
any callback. Per-operation master/cache/address and uncertain-voice checks remain.
Public manual dispatch and whole-song analysis retain full conversion validation;
range restoration remains on its separate synchronous conversion path.

Host ASan/UBSan dispatch PASS (7.767s), including one conversion per preparation
step, repeated readiness, zero startup/control commit conversions,8/16-bit derived
plans, duplicate leases, pitch-control parity, stale/cancel behavior, uncertain
voice/missing control callback atomic batch refusal and pending-stop lease retention.
Staged editor lifecycle suite PASS (29.590s). Native cross-build/editor-main syntax
PASS;172 input hashes/font and candidate verified. Header contract comments were
updated/rebuilt with an identical binary hash, then all current inputs reverified.

Candidate409408 bytes SHA256
80a8835862af33b8db84c91f25911eb70ec64b63efe0f2da7c6e6267f1d16354.
Shared030 run render-files-1790508258023746000 PASS within the coordinated120second
full-suite window, RC0.1255 Fast allocations, zero owned bytes, no Chip fallback.
All timer/watchdog/lease assertions passed, all four DMA channels off. Exact
run/launcher cleanup and independent absence verified; explicitly released to
AmiConnect, no recovery hold or process retry/reset.

Observed native startup costs were9806ticks (one voice) and83774ticks (16 voices)
at709379Hz. Measurements include instrumentation and emulator scheduling; they
remain variable and do not establish a speedup or real-time deadline acceptance.
No audio/card/physical operations, native enhanced PLAY enable or layout changes.
