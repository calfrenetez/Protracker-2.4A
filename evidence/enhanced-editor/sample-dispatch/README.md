# Bounded native sample dispatch

2026-09-24 shared030 run1790246220165249000 passed: unsupported1MiB input
refused without explicit allocation or master/history mutation; donor-only WAV
refused; supported stereo24-bit WAV imported exactly; failed import preserved
redo. Production master allocator reported7Fast/not-Chip allocations, zero owned
bytes. Sample-dispatch/source/WAV/IFF host sanitizer regressions and full native
build pass. Native UI routes to this tested platform function; no fresh UI
acceptance of the unrelated dirty display candidate is claimed. No physical proof.

Old native fallback read unknown input whole into a buffer before rejecting it.
All supported RAW/WAV/IFF/MOD/PP20 paths already have bounded readers; removing
the fallback retains them and avoids needless memory pressure on invalid input.
No runtime-library allocation accounting is implied. Script done, test input
removed, DMAoff verified, exact owned run/launcher cleaned, reservation released.
