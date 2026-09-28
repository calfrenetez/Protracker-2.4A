# Current streaming ownership fixtures

Five shared030 invocations passed in 151.959 seconds: queued song and pump,
consumer and queued song, editor Studio, injected register session, reserved PCM
session. Six distinct binaries; queued song repeats because the existing consumer
runner includes it. planned.json records exact hashes and indexed source tree.
Five executables come from the verified 147-target full build; reserved-session
uses its dedicated SDK-pinned builder, freshly built in the same source export.
Its separate compiler, runtime, SDK and source manifest is embedded in planned.json.

All fixtures returned zero and released all owned Fast memory. Counts are queued
song25, pump4, consumer6, editor171, register19 and reserved3. They cover true24
reference PCM, bounded queue stalls/drain, pending/error cancellation leases,
editor mutation/disposal barriers, prefill/start/readback, retained PCM reservation
through failed reset and quiescence. Each invocation received independent locked
running/all4DMAoff/exact owned path absence checks before the next launch.
AmiConnect received explicit RELEASE after the final cleanup.

The initial host staging attempt failed before any guest/bridge/lock operation:
the reserved-session binary was absent from the full core manifest because it
uses a dedicated build script. That cancelled window and correction are recorded
separately; they are not a guest failure or a guest retry.

No actual input/output device, MMIO, card, listening or physical acceptance.
The physical Amiga remained off. Recording-format development began on the host
while these immutable, previously built streaming executables were running;
this evidence does not qualify those later recording changes.
