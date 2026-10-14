# Native fixture output capture

`tools/native_fixture_capture.py` supplies pure script-generation and parsing
functions for finite native diagnostics. It does not launch a target, grant
ownership, transfer files or release a test window.

A shared Shell capture can contain the native program's output before the
Shell's earlier `Echo` lines. One sampler run returned RC0 and all eight expected
PASS lines, but its old parser rejected that interleaving. The captured result is
preserved as a qualification failure; it is not retroactively relabelled PASS.

The revised contract uses a distinct file inside the caller's unique, owned RAM
directory for native stdout. It refuses an already existing capture before the
invocation. Stack setting, bare Stack query, WARN exits, native invocation and
immediate return-code observation still occur in the same Shell. The caller
validates Shell controls and native output independently:

- Require complete unique Shell controls in order, the exact Stack query and RC0.
- Read the exact native capture pathname's byte count and CRC, bounded to 8192
  bytes, then fetch at most two 4096-byte READFILE chunks.
- Validate the installed hex-dump format, offsets, byte count, ASCII column and
  whole-file CRC. The caller must recheck the capture checksum after reading it.
- Require exactly the expected native marker lines in order, without duplicates
  or additional output.
- Separately verify task absence, remove both the executable and capture from
  the exact owned directory, and complete guarded cleanup and independent release.

Neither parser success nor a script acknowledgement proves cleanup, device
ownership, physical memory placement, output timing or listening acceptance.
Callers retain those responsibilities and must preserve failed-run evidence.

Host checks:

```sh
python3 -m unittest discover -s tests -p test_native_fixture_capture.py
```

Six test groups cover script admission, same-Shell controls, negative Stack/RC
cases, bounded path/checksum validation, malformed chunk rejection and separate
native marker validation. The corrected sampler integration subsequently passed one guarded 68030
Amiberry run with independent cleanup and release; see
[the scoped native evidence](../evidence/harness/native-capture-20261014/README.md).
Real-A1200 integration of this exact candidate remains untested.
