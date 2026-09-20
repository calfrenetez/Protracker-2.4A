# Reference frame clock and transactional timeline, dev29

`src/core/frame_clock.c` adds the explicitly idealized `IDEAL_BPM_Q32` offline
clock: 2.5/BPM seconds per tick at a selected output rate, with Q32 fractional
frames retained across tempo changes. Fixed-point truncation is below one
2^-32-frame unit per tick; exact integer-rational boundaries can therefore differ
by one integer output frame. This policy is documented and is not native CIA
latch timing. `timeline.c` joins it to the flow core as one atomic transition.
The span precedes the completed tick; it uses the prior BPM, preserves startup
lead-in and includes the final F00 boundary. Frame/tick limits do not consume
state or overwrite the previous span. The project remains immutable.

All 39 host checks pass in 45.218 seconds. Exact Python rational arithmetic checks
100,000 changing-tempo ticks at each of 1, 44100, 48000 and 192000 Hz against the
Q32 implementation and stated error bound. ASan/UBSan tests cover tempo changes,
zero-frame ticks, capacity, uint64 overflow, invalid/aliased outputs and failure
atomicity. Timeline tests independently check elapsed spans for F20/FFF/F00,
EE2 delayed processing and a limit immediately before the first row is fetched.

Native run `timing1789910676103949000` passed in 35.952 seconds. The 68000-compatible
binaries run on the emulated 68030. Native clock/timeline assertions passed, and
10,000 changing-tempo intervals per rate (40,000 total) produced the same frame
sequence hashes and sampled exact totals/fractions as the host. Binary identities,
logs and machine settings are in `native-timing.json`; the compiler, flags,
runtime inputs and source hashes are in `core-build.json`.

`PT24GEdit` is byte-identical to dev27/dev28:
`dfa0b3c42c4099705650a0a279711b6e25e7ef0e3e791870c56bac1ab9685888`.
The classic main-screen golden is unchanged. The native workflow never starts
audio; all DMA channels are off. The private launch script was restored, guarded
shutdown completed, and fresh process/HDF/socket checks confirmed release to
AmiConnect at the timestamp in `amiberry-release.json`.

Reproduce with `make test`, `make core-tests`, then reserve the private emulator
before `python3 tools/test_timing_emulator.py`. The script compares native output
against a freshly built sanitizer-enabled host clock oracle and restores the
launch hook on exit.

This is offline timing infrastructure. It does not implement sample mixing,
rendered WAV publication/bounce, a duration/render UI, native CIA-cycle parity,
physical AmiGUS playback or real 68030 performance acceptance. No physical A1200
or AmiGUS was controlled. The next stage is sample/voice mixing on this timeline.
