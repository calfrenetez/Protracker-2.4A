# ProTracker 2.4G — AmiGUS Edition

Build preparation for a native Amiga tracker with 1–16 channels, classic
ProTracker workflow, and exclusive Paula, AmiGUS or MIDI routing per channel.

**Status: handover reviewed; application implementation and build validation
have not started.** This repository currently contains planning material, not
a buildable tracker.

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
user instructions control the work. The current action is processing the
updated handover and preparing its repository commit.

The next build milestone is to pin and reproduce the unmodified native 2.3F
baseline, then establish compatibility tests before enhanced changes.
