# dev51: selected-pattern-row reference rendering

Half-open row ranges pre-roll from row0 without publishing audio, preserving
sample/effect state. Capture starts on first fetched selected row and ends
before the first subsequent fresh row outside selection. Delays and enclosed
loops remain; loops leaving below start end capture. Unreached/invalid ranges
refuse before sink/publication. Internal budgets include pre-roll; output
frame/clip reporting excludes it. Pattern-only and no lead-in required.
CLI --from-row/--to-row combine with --pattern and optional stems. Editor range
controls and selection-driven bounce are not included yet. Main screen unchanged.

64 host regressions passed108.535s. Targeted range test also passed a44.1kHz
source variant leaving nonzero fractional/loop phase after pre-roll, compared
byte-for-byte to the full-pattern crop. Normal fixture covers glides/arpeggio,
delays, loops within/across boundaries, pre-roll cancellation, empty/invalid
refusal, report preservation and full-range identity.

Native range1789941193603618000:5 executions passed77.04s.
Native core checks pass; CLI crop WAV equals host exactly; existing/empty/invalid
outputs refuse without staging residue. Source/build hashes verified.
Release21:55:29UTC after guarded QUIT and fresh process/HDF/socket/launcher
checks; AmiConnect notified. No physical hardware testing. The dev50 shared
folder/ASL empty-directory reappearance limitation remains separate and open.
