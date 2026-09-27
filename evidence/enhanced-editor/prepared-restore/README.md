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

Initial attempt (before the recovery below): coordinated shared030/DevBench run
render-files-1790508818178532000 failed BEFORE launch acknowledgement: bridge
run_script communication timeout. Original result.json remains passed:false.
Read-only shared-lock/live-process/CPU/target guarded inspection found all four
DMA channels off, GET_STATUS Paused=true Config=autosave and the initialized
AmigaBridge shell. Exact staging contains only the candidate, with no test.log,
RC or done marker. An unacknowledged launch may be pending; lack of files is not
proof that it cannot execute when resumed. No resume/reset/retry or cleanup.
At this stage shared030/DevBench remained on a coordinated recovery hold. No physical/card/audio
acceptance is claimed. Candidate and launcher were retained at exact owned paths.

The repository runner now checks GET_STATUS before guest staging and again before
launch, refusing paused/missing/ambiguous running state. Two host tests PASS:
paused/unknown/IPC-failed checks preserve existing guest files; running permits
staging, a later pause refuses launch and retains staged evidence. These tests
use a fake guest and alone do not clear a recovery hold. Status checks cannot
prevent a concurrent pause after the last check; launch failures still retain
owned paths for explicit recovery.

## User-approved resume and verified recovery

The user approved resuming the existing guest to resolve its pending launch.
Fresh AmiConnect/Scott/AmiWBMonitor ownership checks found no conflicting work.
Read-only live identity checks verified68030,2MiBChip/128MiBZ3, the exact shared
Workbench-test.hdf and Dev mount despite GET_STATUS's autosave label. The retained
candidate hash and launcher bytes matched. A single RESUME under the shared lock
returned Paused=false; the ORIGINAL queued launch then executed. No reset, retry,
new launch, physical/card operation or native audio output occurred.

The original fixture finished within the120second observation after resume,
done/RC0, with native timer/watchdog, staged fractional8/16-bit restore, ordinary
command bank and editor ownership assertions passing.1255Fast allocations ended
with zero owned bytes and no Chip fallback. Observed startup duration was9750ticks
for one voice and68945 for16 at709379Hz; emulator/instrumentation variability means
no realtime or speedup qualification. The original launch result remains false.

Completion, exact candidate/launcher/file list, final ownership marker and all
four DMA channels off were rechecked under the shared lock. Logs/RC/done/launcher
were archived before removing only the exact run and launcher. Independent absence
was verified and shared030/DevBench explicitly released to AmiConnect. Recovery
hold cleared. The two retained scripts document that exact recovery only; do not
rerun them or reinterpret the original failed acknowledgement as a successful launch.
