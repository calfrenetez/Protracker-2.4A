# Standalone two-lease HOST model

This test model exercises acquisition and cleanup of two distinct numeric resources. It is separate from production and default native builds. Compile it only with `PT_PRIVATE_TWO_LEASE_HOST_MODEL`; the header rejects Amiga target defines even when that opt-in is present.

Eleven assertion groups cover distinct primitive identity, separately bound server/code/data, complete sibling snapshots, second-acquisition refusal rollback, independent server removal and resource return, once-only cleanup, retained uncertain outcomes and read-only probes. Original registrations, tickets, first ticks and frequencies remain copied values. A probe cannot clear uncertainty or certify QUIET. HOST restoration does not authorize native code/context disposal.

From the repository root, the supported Mac recipe is:

```sh
/usr/bin/cc -std=c99 -O1 -g -Wall -Wextra -Werror -pedantic -UNDEBUG \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DPT_PRIVATE_TWO_LEASE_HOST_MODEL=1 \
  tests/host_two_lease_model/host_two_lease.c \
  tests/host_two_lease_model/test_host_two_lease.c \
  -o /private/tmp/pt24g-two-lease-host-manual
ASAN_OPTIONS=halt_on_error=1:abort_on_error=1 \
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  /private/tmp/pt24g-two-lease-host-manual
```

Use a fresh private output for each deliberately selected run. The exact output is:

```text
HOST two-lease fixture assertion groups: 11
HOST_TWO_LEASE_TRANSACTION_FIXTURE_SOURCE_V1_PASS
```

The checked private V2 build/run passed with these flags and environment. The three adopted C/H bodies are byte-identical to that candidate. This documentary repository-path recipe has not been replayed. The first V1 runtime aborted because this Mac does not support `detect_leaks=1`; that failure remains preserved. No LeakSanitizer qualification is claimed. Comments in the unchanged source describe its original SOURCE-only creation state; current acceptance is recorded in the evidence and contract below.

Six separately executed syntax-only checks confirmed the intended header errors: no opt-in, then `__m68k__`, `__mc68000__`, `__amigaos__`, `__AMIGA__`, and `AMIGA` with opt-in. Each returned compiler RC1 and the corresponding literal header error. These are compiler rejection checks, not native builds or executable tests.

See `docs/HOST_TWO_LEASE_MODEL.md` and `evidence/enhanced-editor/host-two-lease/`. No CIA, IRQ, SDK, real clock, timing, emulator, physical or listening acceptance follows from this HOST model.
