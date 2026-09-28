# Editor recording change barrier

Host: two sanitizer cases pass8.028s. Initial host/cross-build attempts failed to
compile a test using a nonexistent editor channel field; corrected test uses the
actual row navigation state. No initial candidate was run in the emulator.

Shared030 run1790568563778602000 PASS/RC0,64Fast allocations/zero owned bytes.
The actual editor controller refuses edits, undo and disposal until recording
stops. A nonempty stopped recording remains owned until explicit publication or
discard, including failed publication and retry. Successful publication selects
the new master and survives editor undo/redo with exact low24-bit sample data.
Pending/failed detach retains context; faults remain visible after cleanup.

Source14a4e4a60dcf641e83b8b5f568fd0ec09cb7cfc6;124source/3binary hashes verified.
Tested162232byte executable SHA256
6b4133d92682312c8a5ea53e7c3230a3864aafaafa70897841db0b975b8e59cf.
Normal cleanup and independently locked runningguest/all4DMAoff/ownedpathsabsent
verified. Explicit AmiConnect RELEASE; no outstanding process or ownership hold.

Input/card/interrupt callbacks are synthetic; this does not provide native input
controls, device-format negotiation or actual hardware recording acceptance.
The classic display files/layout are unchanged by this increment. Physical Amiga
remains switched off and was not accessed.
