# Pinned repeat handoff

Three targeted host tests PASS (mixer, interval partitioning, sampler ownership),
ASan/UBSan. Pinned native build PASS. No guest/physical run this milestone.

The handoff test verifies exact old/new true24 output across a forward boundary,
simultaneous pins before transition, old-pin release after transition, invalid
pending replacement rollback, repeated replacement without leaking pins, alias
refusal against pending PCM and closing before transition releasing both pins.

Renderer audit: instrument-only changes require this capability. Independent
initial/repeat triggers and effect-to-voice mapping are still pending; no complete
enhanced song playback or live AmiGUS transport/performance claim.
