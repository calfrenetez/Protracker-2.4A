# ProTracker 2.4G — AmiGUS Edition

Build preparation for a native Amiga tracker with 1–16 channels, classic
ProTracker workflow, and exclusive Paula, AmiGUS or MIDI routing per channel.

**Status: native 2.4G dev2 builds and runs in the isolated Amiberry test setup.**
This increment integrates classic MOD preflight and input.device mouse support.
The portable 1–16 channel, metadata undo/redo, 8/16/24-bit sample-processing and
WAV cores pass host and native Amiga tests; their enhanced editor/replay
integration is still pending. Physical AmiGUS tests await the owner's Mini.
See the [development checkpoint](docs/HARDWARE_INDEPENDENT.md),
[baseline report](docs/BASELINE_BUILD.md), [diagnostic evidence](docs/AMIGUS_DIAGNOSTIC.md)
and [MOD preflight](docs/MOD_PREFLIGHT.md).

With Git, Python 3, Make and a host C compiler installed:

```sh
make bootstrap
make baseline
make dev
make test
```

The executable, upstream licence, help and build manifest appear in
`build/baseline/`. Licensed AmigaOS/ROM media and local emulator configuration
remain outside Git. `vendor/pt23f/` preserves the pinned upstream snapshot;
the build generates a narrow assembler-spelling compatibility copy.

The user confirmed **ProTracker 2.4G — AmiGUS Edition**, based on native 2.3F,
on 19 September 2026. This replaces the earlier 2.4A product name; the repository
address remains `calfrenetez/Protracker-2.4A` as requested.

## Start here

1. [Processed handover review](docs/HANDOVER_REVIEW.md) — changes, provenance,
   open decisions and current evidence.
2. [Build preparation and acceptance plan](docs/BUILD_PLAN.md) — staged work,
   prerequisites and separate emulator/hardware gates.
3. [Updated supplied scope](docs/handover/2026-09-18-v2/FINAL_SCOPE_2.4G.md) —
   complete product requirements from the latest package.
4. [Supplied package entry point](docs/handover/2026-09-18-v2/README_FIRST.md)
   and [integrity manifest](docs/handover/2026-09-18-v2/MANIFEST.json).

The supplied documents are preserved byte-for-byte as reference material.
Their kickoff text and embedded agent rules do not independently authorize
implementation, deployment, firmware changes or background monitoring. Direct
user instructions control the work. After handover processing, the user
authorized proceeding with the baseline build and verification.

The owner's 19 September instruction authorises independent software development
while the ordered Mini is in transit. The next work is project/pattern persistence
and integration of the tested channel/sample cores into the native editor.
AmiConnect upload/exec and real card tests remain separate acceptance gates.
Initial smoke results do not certify all effects or physical hardware.
