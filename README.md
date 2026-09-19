# ProTracker 2.4G — AmiGUS Edition

Build preparation for a native Amiga tracker with 1–16 channels, classic
ProTracker workflow, and exclusive Paula, AmiGUS or MIDI routing per channel.

**Status: native 2.3F builds reproducibly and passes an initial AmiBerry smoke
test.** The first native AmiGUSTest diagnostic is built and its absent-library
behavior is verified. Enhanced tracker features and positive physical AmiGUS
tests remain pending. See the [baseline report](docs/BASELINE_BUILD.md) and
[diagnostic evidence](docs/AMIGUS_DIAGNOSTIC.md).
An independent [MOD preflight tool](docs/MOD_PREFLIGHT.md) now covers malformed
file bounds on the host; integration into the tracker loader remains pending.

With Git, Python 3, Make and a host C compiler installed:

```sh
make bootstrap
make baseline
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

The next milestone is broader compatibility/hardening coverage and integration
with AmiConnect's forthcoming authenticated upload/exec capabilities before
enhanced tracker changes. A duplicate hard-coded diagnostic profile is deferred.
Initial smoke results do not certify all effects or physical hardware.
