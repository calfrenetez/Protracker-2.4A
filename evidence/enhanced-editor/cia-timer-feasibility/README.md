# CIA timer-only diagnostic: host/build PASS; native NOT RUN

1 October 2026. The user's existing continuation authority covers this finite
feasibility experiment. The earlier architecture pause was premature and has
been corrected. No production scheduler, priority or timing-tolerance change.

Implemented a diagnostic-only CIA owner, IRQ timestamp handler and fixed
16-observation run on the original 1536-frame/48 kHz grid. The handler reads the
actual EClock and signals its original task. No target timestamp substitution,
Paula writes, audio.device calls, playback or frontend binding. Every Wait also
accepts Ctrl-C and an independent 60000-frame termination request. Only a free
vector with a stopped timer is eligible; occupied vectors and running timers
are refused. Handler/context lifetime lasts until confirmed owned timer stop
and exact vector removal; unresolved closure retains the live task/resources.
Foreign timer registers and ICR bits are preserved. The free owned timer's
stopped control is restored; its programmed count latches are not restored.

Host: seven tests PASS (1.638s), including eight ownership/failure cases,
400000 independent elapsed-clock comparisons, clock/alarm failure boundaries,
and paused/unknown guest plus incomplete/uncertain cleanup refusals. C fixtures
use ASan/UBSan and strict warnings. Python compilation and diff checks PASS.
Pinned m68000 HUNK cross-build PASS; C/assembly offsets and final IRQ
ReadEClock(-60)/Signal(-324)/register preservation verified. Manifest records
project/SDK/compiler/runtime/assembler hashes. Host tests do not execute the
native fixture or establish interrupt latency.

The separately acknowledged candidate fefacaf255b3e5ad0546d6283429f6e3691a4934,
29320 bytes SHA256772422df3e9e7805082325763e1a37cb3719a0df3f866f784c8fbbf17ab9fccf,
was refused before staging because the live shared guest reported Paused=true.
An earlier sandboxed attempt failed process inspection before any guest command.
Neither attempt created run/launcher files or launched the diagnostic. No CIA
vector/timer was acquired. This is NOT a native timing failure or cleanup pass.

Independent read-only verification under the nonblocking shared lock confirmed
the sole expected shared030 process/profile, connected bridge/68030, Paused=true,
all four audio DMA channels off, and exact absence of both attempts' run and
launcher paths. No guest lifecycle or file cleanup was performed. AmiConnect
explicitly acknowledged release of the unused TAKE and has no evidence naming
pause ownership or an authorized resume. This releases our reservation only;
it does not establish global guest readiness. Physical OFF/deferred/unprobed.

After release, added failed-arm retention and all-running-timer host cases and
made closure reporting distinguish never-acquired timers. Final build.json
candidate ad116d4281bd4af3ae32e977214bf6fb43099187 is 29328 bytes, SHA256
78d8dfc8f3f6e89b3bec71b0138c276ab9a1b3817e8f19032f33241b98156305. NOT RUN or
reserved. Any run needs a fresh candidate-specific TAKE and live guards.

Next gate: resolve authority for the unknown shared guest pause, then one fresh
coordinated timer-only run. Do not automatically resume/reset/retry. Only actual
IRQ observations can inform whether strict timing remains feasible. No playback,
audible, frontend, worst-case physical or architecture acceptance is claimed.
