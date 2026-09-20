# Selected-instrument MOD source browser, dev20

LOAD SMP recognises normal MOD/PP20 input and opens a separate read-only source
page; Shift-L requests that source directly. SOURCE and DEST controls select an
instrument and destination slot. IMPORT copies PCM/name/rate/volume/finetune/loop
as one chronological sample undo resource without replacing the current song.
Source memory reserves part of the sampler budget until close/replacement/exit.

All 29 host groups pass. Sanitizers cover every source-load/copy allocation
failure, peak budget, unchanged project/history while browsing, malformed source
preservation, empty-source refusal, slice-reference protection, exact metadata/
PCM copy, deep ownership, source release and undo/redo after closing the donor.
Full/cached planar rendering matches across source changes, empty waveforms,
import/undo/redo and closing. The independent workflow fixture is reloaded between
render and editing tests so owned sample versions are correctly disposed.

Native PTSourceTest repeats ownership/failure checks on 68k. The actual editor
opens a PP20 donor without dirtying the song, cancels/reloads a normal donor,
rejects an empty source, selects instrument 02 and destination 03, imports, undoes,
refuses a corrupt replacement without losing source/redo, redoes and closes the
source. Independent inspection checks name SOURCE TWO, 32 exact signed8 frames,
volume 48, finetune -3 and forward loop 8–24. Native MOD export equals the original
song with precisely that one sample-header/PCM replacement; orders, patterns,
other samples and all remaining bytes match. PTG CRC and byte-identical reopen
also pass, with two normal exits and DMA off. Screenshots are actual native output.

Source audition is not connected. Enhanced PTG donors are unsupported. Empty
source import does not erase a destination. The input file and original current
song remain additional allocations outside the sampler policy budget. This proves
software/emulator behaviour, not physical hardware audio or performance. Amiberry
ownership remains explicitly reserved for successive ProTracker native checks.
