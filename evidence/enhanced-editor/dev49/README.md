# dev49: batch track/group WAV stems

CLI --stems and --groups export an entirely new folder after every WAV has
passed byte verification. Group0 stays individual; selected nonzero group
members combine without widening selection. All masks retain global timing,
mute/solo, pan and gain. Selected MIDI is refused. Cancellation/failure cleans
owned staging, and no-replace directory publication preserves existing or
racing destinations. Accepted main-screen layout unchanged; editor integration
is next, not included here.

63 host regressions passed104.912s, including exhaustive65535-mask partition
checks, group/single PCM equivalence, timing from an unselected track,
cancellation, preflight refusal and a destination created during rendering.
Targeted two tests passed again2.703s after native harness diagnostic change.
Native run stems1789939207429747000:5 executions passed124.224s.
Native and host track WAV files are byte-identical. Native grouped export,
cleanup/cancellation and no-replace directory race tests passed.
Source/build hashes verified. Guarded release21:23:00UTC verified no process,
HDF holder or socket and exact launcher restoration; AmiConnect notified.

First native run failed because test harness used POSIX ./batch on Amiga DOS.
No staging was created. The corrected harness supplies explicit PTDEV:run paths;
no exporter logic was changed to clear that failure. Original failed logs and
release evidence are retained separately. Runner now detects assertion abort
logs immediately, avoiding waiting for a return code from a stopped process.

Reference timing/PCM policy remains distinct from analogue Paula or physical
hardware acceptance. No A1200 or AmiGUS tests performed.
