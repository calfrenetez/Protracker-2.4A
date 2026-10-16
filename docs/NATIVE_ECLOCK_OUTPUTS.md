# Native clock output storage

The owner-thread EClock reader now rejects output spans overlapping its complete
private timer request, message port or the known shared Device header, as well
as its existing handle and mutual-output checks. Refusal occurs before the clock
call and publishes neither output. A successful read still returns the actual
full counter and frequency; there is no epoch, schedule, tolerance or priority
change. The shared header check names only the SDK Device object, not an unknown
complete library allocation. Callers retain the existing original-owner and
serialized lifetime requirements.

The unchanged baseline accepts an aligned frequency output in its own live
request and calls the clock. The preserved regression uses a complete requested
timer object and real fake-Exec open/close sequencing. Thirty-three refusals and
thirty-three subsequent controls cover whole request/port/Device-header/owner
extents, including partial overlaps. Refusals preserve every beforeimage and
make zero clock calls. Existing clock and pending-alarm lifecycle tests pass.

A separate native fixture tests only its own private request, port and handle:
twenty-five refusals followed by twenty-five actual-read controls, retained
counter/rate validity and exact unsubmitted close. It never points an output into
the real shared Device or Exec headers. Unexpected private-storage mutation
retains the owner and process without repair or disposal. The exact entry body
also passes under fake Exec with sanitizers.

Four HOST tests and the pinned one-unit real-SDK compile/link pass. The original
standalone scheduled-output builder stays unchanged; a separate recipe retains
its compiler flags, seven runtimes, full source seal, SDK dependencies, enabled
assertions and HUNK checks. This is not native execution, memory placement,
whole-callchain Stack/WCET or timing qualification. Amiberry and physical results
for this new fixture are NOT_RUN at this milestone. Earlier adjacent clock-pair
observations used their own frozen header and direct assembly reads; they do not
qualify this changed reader. Native scheduled-output providers stay inhibited.

See [HOST and compile evidence](../evidence/enhanced-editor/eclock-output-ownership/host-build.json).
