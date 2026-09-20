# dev42: native E9x/EDx trigger evidence

Ten fixtures (162 ticks per pass), each captured twice with exact equality, plus the unchanged first52
baseline comparison: 21 native captures in 48.769 seconds. The diagnostic binary
is unchanged from dev39; shipping binaries and the accepted screen are unchanged.
Independent Python range interpretation and explicit expected tick lists verify
all captures. Renderer enablement remains unfinished. No physical access.

The initial outer process/HDF check was denied by the sandbox, but orchestration
incorrectly continued. AmiConnect had explicitly confirmed no current/pending
claim; the diagnostic checked socket absence before launch. An approved check
after launch identified PID 93100 with the exact private profile and as the sole
private-HDF holder. This is a procedural limitation, not a passed preflight.
The retained runner now performs process/HDF/socket checks inside main before any
launcher write and stops if process inspection fails. No unidentified process was
stopped. Guarded QUIT and exact launcher restoration were freshly verified; see
amiberry-release.json. AmiConnect received the correction and explicit release.

A regression test injects a denied process inspection and verifies that the runner
raises without a launcher write or emulator process launch.

Validation: full regression 52 tests passed in 88.677 seconds; targeted two-test run passed in 0.013 seconds, including the subsequently added inspection-denial check (53 distinct checks total).
