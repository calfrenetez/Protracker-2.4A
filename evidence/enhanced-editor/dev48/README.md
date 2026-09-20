# dev48: finetune and native note quantization

All16 sample tunings and E5x are supported by the reference renderer. Ordinary
notes select the pinned zero-table index then the active tuning value. Sample
reload restores header tuning; E5 takes effect before same-row notes. Tone
portamento targets retain the native negative-tuning correction; gliss and
arpeggio scan the active tuning, including defined adjacent/last-table overflow.
The complete607-word generated table is checked against pinned assembly.

60 host regression checks passed in105.907 seconds; retained-native comparison
passed in5.232 seconds (61 distinct checks total). CLI publication/regression
check passed again after help-text corrections. Initial two obsolete finetune
refusal assertions failed and were corrected; the development failure log is
retained separately. Sample/effect refusal tests still verify no publication.

Native run finetune1789938144775120000:95 checks in272.705 seconds.
23 fixtures cover all16 tunings, overrides/reload, arpeggio overflow, negative
tone target/gliss and raw-note boundaries.336 ticks per pass, captured twice
exactly; every stored/output period and reference PCM comparison passed.
Source/build hashes verified. Accepted main-screen layout unchanged.
Guarded release at21:07:51 UTC: no process, private-HDF holder or socket;
launcher restored exactly and AmiConnect notified. No physical tests.

The shared core still uses ideal-BPM timing and PCM-rate-at-period428 scaling.
This proves native effect-control parity and reference PCM policy, not analogue
Paula sound, A1200 performance or AmiGUS behavior. Zero sounding periods,
instrument-only events, cross-sample/slice glide handoff and other documented
unsupported inputs remain refused before output.
