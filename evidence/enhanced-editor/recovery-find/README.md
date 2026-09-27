# Read-only recovery discovery and selected restoration

Inspect exactly two snapshot paths in an explicit recovery directory. Validate
one bounded project at a time, requiring matching document identity/source and a
timestamp later than the supplied source timestamp. Corrupt or incompatible
snapshots are skipped; allocation/file/memory capacity failure returns an error
instead of hiding a potentially newer candidate. NONE/ERROR preserve the result.
After explicit selection, staged restoration rechecks metadata before replacing
the current document; changed metadata or any failure preserves the open song.
Success strips recovery metadata, preserves exact masters and marks dirty. No
source or snapshot is modified. Timestamp comparison is a UI hint, not proof
against external edits. Native discovery UI and automatic scheduling remain pending.

Host ASan/UBSan and shared030 passed all cumulative recovery cases including
corrupt-newer fallback, every discovery allocation failure, identity/source and
freshness filtering, changed-selection rejection and exact master restoration.
Candidate88572bytes SHA256
9ce5c96d7f749c0e623a5fda6e9ca34f5fa13e4c2af89616bc1274d5081c8dc1;
all38 dependency hashes matched staged source. Run
render-files-1790553327779289000 RC0,345Fast allocations returned to zero, source
unchanged. Separate guarded identity/running/all4DMAoff/absence check passed;
window explicitly released. No physical or real-audio acceptance.
