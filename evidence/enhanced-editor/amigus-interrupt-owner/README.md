# Interrupt callback lifetime and native call ABI

Pinned driver review found that a failing install can already retain callback
storage, and the removal call has no return status. The new serialized owner
marks the reservation before installation, including partial failure. Ending the
access lease or closing the card is refused until removal has been requested and
a separate bounded quiesce callback returns exactly1. Pending, error and invalid
statuses keep the binding alive. Removal is requested once; stop remains pollable.

Native guarded callbacks use per-context library bases and published -48/-54
vectors. Independent assembly captures a0/d0/d1/d2/d3/a6, including stable callback
entry/data, full32-bit installation errors and both PCM/wavetable selections.
Private fake callbacks are invoked in ordinary task context even after failed
installation. No library is registered or opened, and no real interrupt is installed.
A real device adapter must supply quiescence; no implementation is invented here.
Native PLAY and physical output remain disabled/unqualified.

Host sanitizer suite13 tests and editor/wavetable integration5 tests pass. Native
candidate33056 bytes SHA256
29ec3c43d328fabbdb7266bb3a0d293e37fe0184edfd5d3321530c2dff0c24ce;
all17 manifest inputs match the index. Shared030 run
render-files-1790550269845844000 passed RC0/oneFast allocation/zeroowned bytes/no
Chip fallback, all4 DMAoff. Empty run directories again appeared after runner
cleanup. The hold was retained for guarded exact empty-directory removal and a
separate elevated absence check, then explicitly released. All observations are
retained; shared-folder root cause remains unknown. Physical Amiga OFF/unprobed.
