# Native playback preflight

Host ASan/UBSan preflight and classic MOD round-trip suites pass. Full pinned
native build succeeds, including the existing unrelated dirty display changes;
this is not UI/release qualification. No emulator/physical operation this turn.

Preflight uses a private shallow metadata snapshot without editing master PCM,
events, sample precision or saved channel settings. It retains classic playback
eligibility and explains unavailable Studio/AmiGUS/MIDI routes and sample format
restrictions. MIDI is not promised as renderable audio. High-resolution low bits
remain unchanged after refusal; aliasing the project as snapshot is refused.
The caller returns before halting existing playback on a new-play refusal.

No layout edits, automatic conversion, driver probe, MMIO or backend enabling
were introduced. The messages still need a coordinated native UI check.
