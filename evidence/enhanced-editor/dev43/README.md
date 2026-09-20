# dev43: bounded E9x reference rendering

E9x enabled in shared WAV/sample-bounce rendering for the same mono8/even/no-slice
subset as sample offset. EDx remains refused. Five native dev42 trace fixtures
provide an independent trigger/range oracle for 46080 stereo24 reference frames
on the host. Channel16 tests cover E92, 9xx followed by E92 and invalid-format
refusal before sink/report mutation. Full regression: 54 tests, 91.021 seconds;
final targeted retrigger test also passed after adding offset inheritance coverage.
Native executables rebuilt and manifest/source hashes verified. Screen unchanged.

Native run `retrigpcm1789933684230733000` passed all six checks in 117.384 seconds, matching host output exactly. Stopped audio and CPU/memory/profile details are retained in native/native-retrigger-render.json. Guarded QUIT and fresh process/HDF/socket checks at 19:50:29 UTC verified release and exact launcher restoration; AmiConnect was explicitly notified. No physical tests performed.

User now authorizes physical A1200 testing after feasible emulator checks for the
relevant milestone are exhausted, excluding AmiGUS. Emulator contention alone is
not exhaustion. Coordinate every real-machine window separately with AmiConnect.
