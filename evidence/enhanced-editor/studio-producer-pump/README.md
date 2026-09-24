# Bounded Studio producer pump

Caller-owned pump retains one copied pending block while queue is full and does
not pull again until enqueue succeeds. End drains; stop/failure discards unleased
audio and preserves consumer leases. No device cancellation/concurrency claim.

Synthetic producer test covers repeated stalls, exact ordered24-bit output,
held leases and failure. Complete queued Studio songs match reference at1/17/256
frames including lead-in/pre-roll and tempo/delay paths. Studio regression family
runs together. PTStudioPumpTest and PTQueuedSongTest are native cross-build targets.

No emulator/physical execution in this milestone. Native build includes unrelated
preserved display work and is not editor release/UI or audio-device acceptance.
Producer and queue are not yet wired into native PLAY or a hardware consumer.
