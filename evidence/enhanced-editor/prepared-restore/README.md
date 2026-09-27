# Prepared range restoration — 27 September 2026

Range preparation computes one active voice command per step after cache leases
exist. Commit reuses the private exact snapshot/rate/format/address bank. The
all-voices-idle requirement, master/version/cache checks, whole-batch acquisition
and location validation, uncertain restore and confirmed-stop ownership remain.
Manual public restore retains full validation/conversion. No native PLAY enabled.

Host ASan/UBSan dispatch PASS7.781s: at most one restore conversion per step,
zero during repeated readiness/commit, independent fractional8/16-bit cursor
parity, cancellation/stale refusal and retained uncertain-stop leases. Staged
editor lifecycle PASS29.179s. Native cross-build/main syntax PASS;172input/font
hashes and candidate410260bytes SHA256 verified:
ec2e3528a688184b23f3e3b0f9318bd4ba563aed64812b473e1bc77f07ed2ce9.

Emulator acceptance is PENDING. Coordinated shared030/DevBench run
render-files-1790508818178532000 failed BEFORE launch acknowledgement: bridge
run_script communication timeout. Original result.json remains passed:false.
Read-only shared-lock/live-process/CPU/target guarded inspection found all four
DMA channels off, GET_STATUS Paused=true Config=autosave and the initialized
AmigaBridge shell. Exact staging contains only the candidate, with no test.log,
RC or done marker. An unacknowledged launch may be pending; lack of files is not
proof that it cannot execute when resumed. No resume/reset/retry or cleanup.
Shared030/DevBench remains on a coordinated recovery hold. No physical/card/audio
acceptance is claimed. Candidate and launcher are retained at exact owned paths.

The repository runner now checks GET_STATUS before guest staging and again before
launch, refusing paused/missing/ambiguous running state. Two host tests PASS:
paused/unknown/IPC-failed checks preserve existing guest files; running permits
staging, a later pause refuses launch and retains staged evidence. These tests
use a fake guest and do not clear the current recovery hold. Status checks cannot
prevent a concurrent pause after the last check; launch failures still retain
owned paths for explicit recovery.
