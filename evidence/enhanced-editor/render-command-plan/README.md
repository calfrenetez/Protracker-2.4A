# Explicit renderer command plans

The audited effect body now optionally records ordered trigger, segment, repeat,
stop and final step/gain operations in a bounded caller-owned plan. PCM remains
borrowed; Studio master pinning and full song dispatch are not integrated here.
Failure clears the plan count and requires session abort.

Validation: complete host suite passed, including the new sanitizer-enabled
16-channel command replay/audio-equivalence and partial-failure discard fixture.
The pinned Amiga cross-build passed. The new fixture is host-only; no emulator or
physical test was performed for this milestone. Native build includes unrelated
uncommitted display work and is not editor release or UI qualification.

Logs and source hashes accompany this record. No live AmiGUS transport, physical
memory acceptance or real-time CPU/deadline claim follows from these checks.
