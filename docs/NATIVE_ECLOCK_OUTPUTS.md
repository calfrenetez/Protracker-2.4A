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
were NOT_RUN at that host/build milestone; completed results follow below. Earlier adjacent clock-pair
observations used their own frozen header and direct assembly reads; they do not
qualify this changed reader. Native scheduled-output providers stay inhibited.

See [HOST and compile evidence](../evidence/enhanced-editor/eclock-output-ownership/host-build.json).


## Exact native qualification —16 October2026

The exact12,500-byte fixture now passes once in both the shared68030 Amiberry
profile and the real68030 A1200/Exec47.13/PAL:25private-storage refusals and25
subsequent actual-clock controls, RC0, one exact success marker and reported
normal unsubmitted-owner closure. Both observe262144-byte task Stack; the252-byte
maximum bounded local frame does not prove aggregate Stack or WCET.

Each run retains exact one-input/eight-execution-file custody, independent full
RAM/task/staging absence, unchanged saved installed inputs, disconnected transport,
separate observation/public release, only its own service shutdown and a separate
final idle check. Physical promotion binds six exact emulator records. Safari
remains authenticated in View Only before and after physical execution; no Amiga
browser input/control is acquired. The physical test never directs outputs into
the shared Device/Exec headers; those refusal cases remain HOST-only models.

The initial HOST factory check could not load an offloaded certificate file;
its failure is retained. Read-only file hydration then loaded121CAs and the full
HOST check passed, with no target operation from that failure. A separate composed
physical packet-path assertion also stopped before any service, lease or target;
its original source remains preserved. No failed target transaction was replayed.

These results qualify the changed task reader's private output-storage checks,
actual read controls and resource lifecycle. They do not establish arbitrary
caller/corrupted-owner safety, all task-clock call paths, whole-library protection,
physical placement, scheduled activation, exact musical timing, IRQ/WCET, audio
or listening. Existing scheduling and native-provider inhibition are unchanged.
See [emulator result](../evidence/enhanced-editor/eclock-output-ownership/emulator.json)
and [A1200 result](../evidence/enhanced-editor/eclock-output-ownership/physical.json).
