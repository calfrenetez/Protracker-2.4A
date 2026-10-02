# Read-only Mini wavetable status candidate

Implementation is committed. Host `tests/test_native_wavetable_read.py` and
`tests/test_physical_discovery_guard.py`: eight checks PASS, including ASan/UBSan,
unsafe-offset refusal, lease/descriptor loss latching, exact native fixture/mode
gates, incomplete status/release evidence rejection, and finished-script release
uncertainty retaining target/files without cleanup or switching.

Pinned native cross-builds PASS:
- AmiGUSTest0.9:27916bytes, SHA076b2e53289bd4cf728657a53d17fa3007a3a768a68d9f10f6e943ebb9320119.
- PTWavetableReadTest:14144bytes, SHAa79ded73db5a2534a4bccfbd1ac0e5217e00bf18681f7c8a0eb954ba985aa6c5.

Exact shared030 suite `render-files-1790931158760994000` passed all15 cases:
six native fixtures RC0 and nine real diagnostic missing-library modes RC5.
The reader fixture verifies eight global reads, unsafe-offset refusal and latched
lease/descriptor loss using synthetic storage. AmiGUSTest0.9's real
`--wavetable-status` mode correctly skips when amigus.library is absent.

The runner's initial two-second cleanup observation passed, but the separate
independent check found the exact run directory reappeared. That failure and the
inventory are preserved. The15 expected empty nonsymlink case directories were
removed once by separately guarded nonrecursive recovery; ten-second absence and
a subsequent independent check passed. Original solePID79263/owned profile/
localhost/running68030/allfourDMAoff and exact run/launcher absence were verified.
Explicit RELEASE and recovery-hold clearance were sent to AmiConnect.

**No physical status run occurred.** Another task's failed physical playback and
temporary mute remain distinct and untouched. Fresh physical ownership/recovery
coordination is required before any separately scoped read-only observation.

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
PASS. The exact suite subsequently executed with this preflight before Guest
construction. See the native and independent cleanup records above.
