# Bounded recovery snapshot retention

A session creates an exclusively new app-owned directory and alternates two new
snapshot paths. It retains the current verified copy until the next fully
publishes, then removes only the prior file it created. If cleanup fails, both
copies remain; further publication refuses to remove the current good copy for
space. Unowned files are never removed to make room. Explicit discard removes
only owned files and an empty directory; foreign entries prevent directory removal.
Identity and increasing timestamp guards support later recovery selection.

Host sanitizers and the corrected shared030 fixture pass. Candidate83784bytes
SHA256a7bd11c5d141f8e885bdc6c8a72fa9e093a71ead0024ba7bc28b74c64b1265ae,
35 dependency hashes match staged source. Run render-files-1790553086968461000
passed RC0,138Fast allocations returned to zero, original fixture unchanged.
Separate guarded absence check passed with all4DMAoff; explicitly released.

The original run1790552525524101000 FAILED and timed out after an assertion.
The fault injection used an empty directory: AmigaDOS unlink removed it, unlike
POSIX. The corrected injection uses a nonempty owned directory. Original failure
and process diagnostics are retained under prior-failure; exact staging/binary
remain archived locally in build/dev/render-files-1790552525524101000/retained-staging.
The aborted fixture did not prove allocation release. A separately coordinated
recovery-only owned stop/start cleared the uncertain guest allocation state;
archived hashes were checked before exact cleanup while stopped. Independent
post-start identity/bridge/process/DMA/absence checks passed. No corrected test
ran during recovery. The failed binary remains unqualified.

No physical hardware/audio proof. Native automatic scheduling and recovery UI
remain pending; this change supplies the bounded file-retention owner.
