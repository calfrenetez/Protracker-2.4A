# Sample attributes and shared PCM ownership, dev23

FINETUNE/VOLUME arrows and Alt-up/down or Alt-right/left edit bounded sample
metadata. Clicking the sample-name strip or Control-Shift-N opens a 31-character
modal name entry. These changes join chronological undo and PTG/MOD persistence.
The accepted main-screen golden is unchanged. Name-entry redraws update the
sample strip even when no auxiliary panel is open; track names retain 15 chars.

Metadata versions now share a flat immutable PCM/marker backing owner. The first
edit of document-backed data captures one owned baseline; later metadata edits
allocate only a version header. Existing sampler-owned PCM needs no first copy.
Journal eviction, failure, PCM edits and mixed undo retain/release these owners
without mutating shared data or creating nested backing chains. Identical owned
PCM/marker pointers avoid redundant byte comparison; full validation still runs.

All 31 host groups pass. The new sanitized ownership test covers invalid metadata,
every allocation failure for initial and later edits, no-op/capacity preservation
of redo, 200 metadata edits, bounded retained memory, shared 24-bit stereo PCM,
slice references, independent reverse PCM, mixed undo/redo, exact project reopen
and zero live allocations at teardown. The host uses 65,536 PCM values; the native
run uses 4,096 for bounded validation time. The initial fixture incorrectly
assumed empty-slot volume was 64; it now explicitly establishes that test value.
Controller/render checks cover name length, cancellation, mouse parameter targets,
undo, and full/cached planar equivalence.

Native PTSampleAttributesTest repeats the ownership/fault/history checks. The
actual editor starts Paula playback, changes volume (stopping the old snapshot),
sets volume 22 / finetune +1 / name BASS, fades PCM, renames that version,
and undoes both in their real order. Cancelled name entry preserves redo. The
exported MOD is compared byte-for-byte with independently modified original
header bytes: only sample name, volume and finetune differ. PCM, notes, orders,
loops and all other bytes match. PTG CRC and byte-identical reopen/resave pass;
both editors exit normally and audio DMA is off. Screenshots are native output.
The initial UI run expected reversal of the symmetric baseline fixture to change
PCM; the correct no-op preserved revision 4. That owned harness was interrupted
with SIGINT and guarded cleanup, then the next run used a nontrivial fade. Its
file assertion assumed source volume 64; the baseline is 24, and the correctly
saved 22 was independently inspected. Final expectations derive volume from the
actual input header. Both earlier harnesses released the owned emulator.

Metadata edits still validate PCM, so large-sample latency on a real 68030 is not
proven. Names exceeding classic MOD's 22 characters require PTG or an explicit
future lossy conversion. Physical A1200/AmiGUS tests are NOT RUN. AmiConnect
requested its next native window during this run; explicit release is recorded
separately after process, HDF and socket checks.
