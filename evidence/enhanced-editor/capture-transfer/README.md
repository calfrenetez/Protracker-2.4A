# Recording buffer ownership transfer

Finished capture with the sampler allocator transfers the unique PCM allocation instead of allocating a second full copy. The sampler charges the entire capacity; publication failure retains capture ownership for retry. Different allocator identities use the existing copy path.

Seven targeted host tests passed with sanitizers (24.876 seconds). They cover refused large allocations, full capacity accounting, allocator mismatch, every append allocation failure, history refusal, exact low bits, metadata sharing, undo/redo, and retained playback pins after current/history release.

All 147 Amiga targets built from the indexed snapshot. The 350 indexed source hashes, one generated source and 147 binary hashes were verified. An initial host invocation preceded archive extraction completion and compiled nothing; its error is retained separately.

Full host suite: 207 cases across 172 modules passed in 258.551 seconds. Results and individual logs are recorded alongside this file. The staged-index helper used this exact tree. The committed-reference helper was additionally rerun against product commit 2a9bcfd and passed (one test, 7.434 seconds); its separate log closes the prior-HEAD coverage gap in the full suite.

Emulator execution is pending. The preceding format-gate attempt was refused before guest operations because the shared 030 identity was unavailable; this candidate has not been executed in the emulator. No retry, lifecycle operation, physical probing, or device capture occurred.
