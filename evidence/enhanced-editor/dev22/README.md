# Pattern slice assignment, dev22

EDIT OP. > NOTE / Control-I edits a normal sample note's stored slice reference.
0000 means whole sample; 0001 identifies the first marker. The displayed range
runs to the next marker or sample end, with its end frame excluded. USE SMP
explicitly attaches the selected marked sample to a normal note. Blank/MIDI/OFF
notes and absent/out-of-range markers are refused for nonzero assignment. Clearing
removes only the slice reference. The original main-screen golden is unchanged.

All 30 host groups pass. Sanitized tests cover ordinal/bounds checks, modal target
isolation, empty-slot refusal, instrument-zero note attachment, preservation of
pitch/effect/velocity/PCM, cancellation without losing redo, chronological undo
across marker and note edits, referenced-marker protection and PTG reopen.
Full/cached planar displays agree through note-page navigation, number entry,
clear/undo, sampler round trips and menu closure. Invalid long channel-number
proposals now stay within their existing button width. A missing WAV include in
the new test harness was corrected before the complete passing test run.

Native PTNoteSliceTest repeats the controller/history checks and writes an input
fixture. Actual Intuition input assigns marker 3, undoes, refuses marker 4 without
losing redo, redoes, and selects marker 2. The sampler refuses deleting referenced
markers. After clearing the note reference, marker deletion succeeds; shared undo
then restores markers followed by the note reference. Channel 5 starts with an
instrument-zero sample note: USE SMP attaches sample 1 and slice 1 is assigned.
Independent expected-byte construction verifies the complete saved PTG: only
those selected event instrument/slice fields and CRC differ, while PCM, markers,
effects, velocity and unrelated project bytes remain identical. Reopen/resave is
byte-identical. Both native editor sessions exit normally and audio DMA is off.

This establishes editing, persistence and ownership semantics. It does not add
AmiGUS slice playback, high-resolution audition or MIDI slice-trigger mapping.
Strict classic playback/MOD export remain unable to represent sliced data.
Physical A1200/AmiGUS acceptance is NOT RUN. The private Amiberry window is used
under the continuing explicit ownership handover from the AmiConnect thread.
