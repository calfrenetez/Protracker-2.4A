# dev44: bounded EDx reference rendering

EDx enabled in shared WAV/sample-bounce rendering with deferred sample trigger,
stored table period separated from hardware output, and retained vibrato phase.
ED0, no-note EDx, delays beyond speed, pattern-delay repeats, saved offsets and
forward loops are checked against six retained dev42 native traces (63360 frames).
The existing mono8/even/no-slice bounds apply; delayed cross-sample changes while
sounding are refused in measurement. Broader formats and handoff remain open.

55 host tests passed in 93.878 seconds, including sanitizer checks and the
portamento forced-retrigger mutation. Additional state assertions cover pitch
write timing, vibrato phase and cross-sample refusal. Build/source hashes verified.
Native run delaypcm1789934387278909000: all seven checks matched host output in
134.362 seconds. Exact report and build hashes retained. Guarded
QUIT and fresh release checks at 20:02:50 UTC confirmed no process/HDF/socket and
exact launcher restoration; AmiConnect received explicit release.

Accepted screen unchanged. No physical testing performed. A1200 testing remains
authorized after feasible emulator checks are exhausted, excluding AmiGUS.
