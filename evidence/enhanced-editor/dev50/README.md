# dev50: native editor stem export

RENDER WAV panel adds STEMS (S) and TRACK/GROUP STEMS (O). Existing WAV/bounce
controls remain; settings do not mutate project/history. ASL asks for a new
folder name, cancellation preserves edits, and successful export reports stem
count and clipping. Main-screen golden remains unchanged.

63 host tests passed103.888s. Production native RAM-disk run
stemsui1789940350255927000 passed98.646s: requester/mixing
cancellation, group-mode panel, four byte-identical host/native WAVs, unchanged
dirty state, exact original PTG after undo/save and clean exit. Native List ALL
shows only input and completed stem outputs, with no cancelled/temp directories.
Screenshot inspected. Production build contains no temporary tracing flags;
source/build hashes verified. Release21:41:27UTC freshly verified no process,
HDF holder or socket and exact launcher restore; AmiConnect notified.

## Retained shared-volume limitation

Two earlier editor runs on Amiberry's PTDEV shared host folder exported correct
WAVs but failed cleanup acceptance: empty cancelled staging directories were
absent immediately after cancellation and reappeared during the next batch.
Diagnostic logs show native DeleteFile followed by Lock=0/IoErr205 for both
paths, plus another absent Lock before next batch, and no application CreateDir
for those old paths. Host snapshots are empty at44.873/46.019s and show the old
empty directories at91.781s. The same issue did not occur on RAM: with the normal
build. The exact emulator/filesystem cause is not established. Do not present
shared-folder cancellation cleanup as fully accepted. No product workaround
or forced deletion of recreated paths was added. Diagnostic-only source copies
and build metadata are retained here; shipping source has tracing removed.

No physical A1200/AmiGUS testing or analogue audio acceptance is claimed.
