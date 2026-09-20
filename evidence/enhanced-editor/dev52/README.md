# dev52: marked-row editor WAV, stems and sample bounce

MARKED ROWS (E) snapshots normalized rows/tracks from the current pattern,
switches to pattern scope and disables lead-in. Missing/wrong-pattern marks
leave settings unchanged. Later cursor/mark movement does not alter snapshot;
changing pattern invalidates it. Track mask remains adjustable. P/E clear range
mode as documented. All three output paths share the same options/pre-roll.
Bounced sample names identify inclusive hexadecimal rows. Settings consume no
history; successful bounce is one undo step. Accepted main-screen golden unchanged.

64 host checks passed115.118s. Targeted editor regression passed6.327s after
shortening three new status strings to fit the existing field. Final native RAM:
run rangeui1789942081736444000 passed140.094s. Marked rows01–02
produce11520 frames; WAV and stem bytes match the host crop exactly. New slot32
contains exact true24 stereo PCM and expected metadata only. Cancellation before
and after undo preserves project and redo; redo/save/reopen are exact. Native
List ALL shows no temporary staging. Final screenshot inspected, text fits.
Initial successful run retained separately; only status text changed before
final replay. All current source hashes verified. Guarded release22:10:53UTC,
fresh process/HDF/socket/launcher checks and AmiConnect notification retained.

No physical tests. Reference timing/pitch policy and dev50 shared-folder cleanup
limitation remain; this proves native RAM-disk workflow, not A1200/AmiGUS acceptance.
