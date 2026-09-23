# Studio mixer session core

Three targeted host tests PASS (new session plus existing voice suites), using
ASan/UBSan. Pinned Amiga build PASS including PTStudioMixTest. No guest or physical
execution this milestone. The native editor does not yet use this new API.

Tests retain an old pinned version after retirement, refuse new retired triggers,
roll back invalid replacement, compare successive true24 blocks with reference,
reject output/master alias, release natural completion and all16 pins on close,
exercise saturation and allocator refusal, and preserve source low bits.

This provides an incremental voice mixer and lifetime callback contract. Sampler
master-version provider, tracker scheduler and AmiGUS PCM transport remain to be
integrated. No real-time16-voice/performance or complete Studio-mode claim.
