# Synthetic recording storage

The bounded software collector preserves signed 8/16/24-bit mono/stereo PCM,
refuses malformed or overrun chunks without publishing a partial recording,
and appends a finished recording as one undoable master transaction. Allocation
failure leaves the collector and existing project/history intact for retry.

Host: capture and sampler sanitizer cases pass (two tests, 6.489 seconds).
Native shared030: run1790567031839409000 passes with 68 Fast allocations and
zero owned bytes. Exact master samples survive undo/redo and project save/reload.
Independent cleanup confirms running guest, all four Paula DMA off and exact
run/launcher absence; the coordinated window was explicitly released.

The combined build covers PTCaptureTest, PTExecCaptureTest, PT24GEdit and the
DMA guard: 165 source dependencies, one generated font header and all four binary
hashes verified. The captured fixture bytes match that final combined manifest.
PT24GEdit is byte-identical to the earlier qualified stopped-capture editor.
The manifest now includes compiler-discovered transitive headers/included C
fixtures and separately hashes generated inputs. An initial capture-only manifest
was overwritten by the editor check; the combined rebuild resolves that host
bookkeeping issue without changing the tested capture executable.

These are synthetic input and production Exec-allocation checks, not actual
recording, format negotiation, input-device start/stop or physical acceptance.
The Amiga is switched off and was not accessed. A recording lifecycle owner is
still required before a real backend can be attached safely.
