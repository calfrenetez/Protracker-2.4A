# Read-only Mini wavetable status candidate

Implementation is committed. Host `tests/test_native_wavetable_read.py` and
`tests/test_physical_discovery_guard.py`: eight checks PASS, including ASan/UBSan,
unsafe-offset refusal, lease/descriptor loss latching, exact native fixture/mode
gates, incomplete status/release evidence rejection, and finished-script release
uncertainty retaining target/files without cleanup or switching.

Pinned native cross-builds PASS:
- AmiGUSTest0.9:27916bytes, SHA076b2e53289bd4cf728657a53d17fa3007a3a768a68d9f10f6e943ebb9320119.
- PTWavetableReadTest:14144bytes, SHAa79ded73db5a2534a4bccfbd1ac0e5217e00bf18681f7c8a0eb954ba985aa6c5.

**No emulator execution or physical status run has occurred for these candidates.**
AmiConnect reported the soundcard-driver task still owns its shared lock/physical
Safari playback window; no release/control/restoration acknowledgement recorded.
ProTracker stayed host-only, acquired no target/card/browser control and has no
Task/IRQ/IO/restoration/recovery hold. The exact15-case native suite and separate
independent original-guest/DMAoff/path-absence check are pending fresh release.
Physical status reads require that exact evidence plus a separate fresh claim.

The actual probe reserves only WAVETABLE and reads exactly the eight global
IRQ/mask registers. No register writes, sample upload, bank selection, AHI unload,
voice start/stop, interrupt, installed file/configuration changes. Own release is
positively confirmed; ambiguity retains the native Task/library/owner. The map
has no documented sample-RAM readback/usage port. Status snapshots do not prove
capacity, ordering/completion, all voices stopped, playback or human listening.

The DevBench runner still needs an exclusive shared window and must not retarget
another chat's connection. The user's independent Safari route remains available
subject to separate live physical ownership/control guards and View Only release.

## Host preflight follow-up

`diagnostic_candidate_guard` now verifies all seven distinct suite binaries,
manifest lengths/hashes, transitive source inputs in both working tree and HEAD,
and pinned SDK lock/header hashes before any Guest construction or guest access.
The real current artifact preflight passes; a temporary Git fixture confirms
changed bytes, dirty source, rewritten manifests with uncommitted sources, missing
inputs/provenance and changed SDK inputs cannot qualify. Nine combined host checks
PASS. No new target/lock/control access occurred. AmiConnect's fresh check still
reports the soundcard-driver Safari/playback reservation unreleased; Scott's new
emulator window request is also pending. Exact suite execution remains pending.
