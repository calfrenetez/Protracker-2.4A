# AGENTS.md

Read `FINAL_SCOPE_2.4G.md` first. It is the sole authoritative requirements document.

Key rules:
1. Preserve ProTracker 2.3F usability and classic 2.x appearance.
2. Each of 16 channels has exactly one route: PAULA, AMIGUS or MIDI. Maximum four Paula routes.
3. Support full-size Zorro AmiGUS and AmiGUS mini through a common backend.
4. Preserve a 68000-compatible hardware-voice path; optimise Studio mode for the ACA1234 68030/50 without requiring an FPU.
5. Establish AmiBerry and AmiConnect real-hardware automation before invasive changes.
6. Build/use a scriptable AmiGUSTest diagnostic.
7. Benchmark hardware claims rather than guessing.
8. Keep classic MODs genuinely compatible; never silently discard enhanced data.
9. Do not add features listed under Explicit exclusions.
10. Keep changes staged, testable and recoverable. Maintain a known-good build.
