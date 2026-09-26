# Selected-track/stem EFx qualification — 26 September 2026

Host sanitizer checks verify exact shared-sample PCM, global mute/solo clocks,
individual/grouped stems, all allocation failures, cancellation and cleanup.
Full `make test` passed:156 tests in361.115s plus compiled core checks, exit0.
Canonical full native build passed; compiler/runtime/source hashes in core-build.json.

First attempt refused before guest staging because no shared030 process existed.
No test ran and no guest path was created. Separately coordinated supported
startup then returned Amiga alive; no running/unsaved guest was reset.

Core run1790461885599365000 passed selected/muted shared-sample PCM, budgets,
allocation failures and cancellation. CLI run1790461954299463000 produced two
17280-frame stereo24 WAVs exactly matching every host reference byte, preserved
the shared-sample MOD fixture, and refused an existing output directory (RC20).
Both windows passed completion, all4 DMA-off and exact cleanup, then were
explicitly released to AmiConnect/AmiWatch. No physical or analogue acceptance.

Native editor EFx integration and queued Studio EFx remain unfinished.
