# Bounded private timer transport

1 October 2026. The new owner-thread transport connects the sampled-clock mixed
owner to a private nonblocking alarm pump. Every service performs at most one
poll, one bounded owner service and one future alarm submission. Preparation or
elapsed-work debt returns PREPARING so the caller continues without sleeping;
otherwise the next wake is bounded by a 1–256-frame quantum and the next musical
boundary. Each completed alarm causes honest clock resampling. Strict late,
skipped or unprepared boundaries fail; there is no catch-up or clock rebasing.

A valid begin adopts exclusive owner/timer control even when clocked begin fails.
Failures latch and attempt retained reader stops once. Pending or uncertain IO,
reader barriers and counter closure retain the corresponding caller-owned storage
until explicit close confirms release. The private counter closes only after the
alarm and sample owner both close. DONE also retains resources for explicit close.
The API is serialized, noncopyable and callback contexts must remain alive.

The OS2+ adapter owns private UNIT_ECLOCK and UNIT_WAITECLOCK requests. It exposes
only its pending signal and uses CheckIO before WaitIO; it never waits on unfinished
IO. It does not install an editor event loop or bind output devices, DMA or IRQs.

Four related ASan/UBSan host modules PASS in23.753s, including240 mixed scenarios.
The39 new pump scenarios span8/16/24-bit masters: complete two-interval pumping,
bounded wakes, pending polls, arm/poll/reader failures, changed/regressed counters,
late startup, low-frequency skipped startup, missed live boundaries, owner identity
changes, latched failure, DONE and retained alarm/reader/counter shutdown. Host
injected-clock proof is separate from native resource qualification.

Indexed source tree2cd2a67ef0cc3a69dcb91892fb44467477f35183:165 source hashes verified,
zero generated sources and three verified binaries. PTExecMixedOwnerTest252208
bytes SHA256bbfbf9665aa71464fba4f8cf10502d715f67a1f8b252e8554604e3b41dfb388a.
ONE shared030 run1790839098210915000 PASS43.402s outer:60 native scenarios, comprising
prior18,39 injected pump cases and3 actual private pending-alarm cancellation/
reader-retention cases. These three submit a future one-second startup alarm,
confirm pending signal, refuse unconfirmed reader closure, then cancel/poll and
close every request/counter/port/device without starting any output voice. Native
fixture-only Delay(1) permits bounded cleanup retries; the production API has no
blocking wait on unfinished IO.

669 Fast allocations/zero owned bytes; every sample/cache/master/reader/lease/pin
and timer resource closes. Normal done/rc0/exact owned path cleanup and independent
subsequent locked process/profile/bridge/68030/running/all-four-DMA-off/run+launcher
absence PASS. Bounds90s fixture/140 runner/200 outer within coordinated240-second
reservation. Explicit release acknowledged by AmiConnect and forwarded to peers after
independent proof. Initial
system-Python host import failed before any staging/guest operation; corrected to
shared harness runtime. No native retry, lifecycle/reset, output/MMIO/DMA/audio,
screenshots/system-clock changes or physical operation.

All1341 separate instrumented component/accessor/running observations and outliers
remain in the raw log; they are not extra native scenarios. Pending cancellation
qualifies resource handling, not wake punctuality, real-time playback or listening.
Editor PLAY/event-loop binding, production output/device/IRQ binding and audible,
pressure/endurance and physical acceptance remain requirements. Hardware is off
and deferred. Earlier failed bulk window remains separately recorded as failed.
