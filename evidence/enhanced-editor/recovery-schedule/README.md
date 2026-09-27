# Recovery scheduling and undo identity

The no-I/O scheduler skips clean work and previously published revisions, waits a
configurable interval, and backs off a full interval after failed publication.
Continuous edits do not postpone due work. Playback/capture/transaction safety is
supplied by the caller; unsafe periods defer writing. Disabled, unknown/volatile
media and unapproved removable media never invoke the snapshot callback. Time
rollback/overflow rebase the interval; callback reentrancy is refused. Exactly1
confirms publication; zero, negative and unexpected positive results retain the
last successful revision. Caller must reset the owner on document changes.

Recovery metadata now treats revisions as identities rather than ordered numbers.
Undo to revision0 after saving revision9 is dirty and can be saved/restored without
changing master data. Host ASan/UBSan and native fixture both passed. Native75768
bytes SHA25657663499e154ffbcd58db1292e806b2a4cf4a16c22d0e0f5a3419db6a5a12e2c;
all32 dependency hashes matched the index. Shared030 run
render-files-1790552241625472000 RC0,108Fast allocations/zero owned, exact original
preserved. Guarded and separate independent cleanup checks passed withall4DMAoff;
window explicitly released. No physical/audio acceptance. Native scheduling,
retention, startup discovery and recovery-selection UI remain to be connected.
