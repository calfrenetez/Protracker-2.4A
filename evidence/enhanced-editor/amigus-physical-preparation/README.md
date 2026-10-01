# AmiGUS physical discovery preparation

1 October 2026. User confirms hardware available and authorizes ProTracker/AmiGUS
physical testing. No physical run in this preparation record.

Current committed production library adapter rebuilt with pinned GCC6.5 and
-fbbb=-, SDK and runtime inputs recorded; all compiled project input hashes verified
against ce8019c. Candidate15524 bytes,
SHA25639036e5f836766ea51bc5461cae0c506b5153d0dfb8c0e177e6835bee1af272e.
Three ASan/UBSan reservation/session/partial-interrupt host tests PASS. This exact
candidate is NOT yet emulator-qualified: older September discovery evidence has
a different executable hash and cannot promote these bytes.

Next: separately coordinated shared030 missing-library probe and independent
cleanup, then fresh physical TAKE/target/bridge identity and RAM-only discovery.
It opens amigus.library twice, enumerates at most16 distinct cards, reports PCM
capability and closes each library reference. OpenLibrary may initialize the
installed driver; this is not an electrically passive hardware observation.
Reserve/release callbacks disabled. No MMIO, codec ownership, interrupt, firmware,
driver installation, network/startup changes or playback. Positive card discovery
would not qualify reservation, cache upload, sound or physical timing.
