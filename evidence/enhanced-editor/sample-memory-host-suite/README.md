# Integrated sample-memory host sweep

All168 host test modules pass after running two Git-dependent checks through
their normal checkout isolation helpers. The initial exported-tree sweep ran166
modules successfully and two archive-context errors; those errors/logs remain
preserved. The two corrections both pass, including editor wavetable, Studio,
invert/reference and edit/undo/disposal ownership checks.

The main sweep used the isolated indexed candidate from odd-frame Paula testing,
with two host workers and180seconds maximum per module. The separate helpers
read committed/indexed sources from the real checkout, preserving unrelated
working display edits. Source/test/helper comparison against HEAD04d1f74 is exact;
the newer commit only adds capture-runner/evidence/documentation changes.

These are host checks, including comparisons with previously recorded native
trace oracles. This sweep does not execute the emulator, access physical hardware,
or qualify sound, MMIO, real-time performance, positive AmiGUS or MIDI devices.
The independently coordinated odd/master/native/editor emulator runs have their
own evidence directories. No claim that every scoped product feature is wired
or complete follows from these tests.
