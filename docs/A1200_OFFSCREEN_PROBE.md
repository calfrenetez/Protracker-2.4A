# A1200 offscreen editor probe

`PTViewProbe` is a finite, no-argument native diagnostic prepared for the
AmiConnect profile-1 test runner. It compares all 163,840 bytes of four-plane
640x512 output after 24 cursor-down steps using full and incremental drawing.
The steps cross the visible-row boundary. The module and pinned font are built
in; no project path or command argument is needed.

It reports full and incremental elapsed 50 Hz DOS ticks and exact pixel equality.
Timing is a measurement, not a pass threshold: scheduler load and tick resolution
can obscure small differences. Success requires identical pixels and nonnegative
time. This measures CPU offscreen drawing only, not display blits, input latency,
Paula sound, AmiGUS, or physical user acceptance.

## Execution review

The entry point is `tests/native_view_probe.c`. It loads an embedded silent
four-channel MOD into private allocations, initializes the production editor,
and calls its drawing/controller functions. It opens no screen, window, audio,
CIA, input device, network connection, or project file; it starts no subprocess.
The only output is at most two short stdout lines. It checks Ctrl-C between each
of the 48 bounded frames, disposes the first editor before reinitializing, and
releases editor/document/pixel allocations on normal completion and cancellation.
There is no force-kill or persistent configuration change. A stalled individual
frame is not interrupted by this cooperative check.

Build with `make core-tests`; `core-build.json` records source, compiler/runtime
and executable hashes. `test_view_probe.py` executes the same entry point with
host DOS/Exec shims under address/undefined-behavior sanitizers, including a
Ctrl-C during the second drawing pass. Host tick values are deliberately synthetic
and are not performance evidence. The existing editor golden test verifies the
accepted screen remains unchanged.

Run `tools/test_view_probe_emulator.py` only after reserving the private emulator.
It checks the current source/executable manifest, uses a 150-second deadline,
restores the launcher and closes only the guarded emulator. Independently verify
process/disk/socket release and notify the coordinating task afterward.

## Physical execution gate

No physical result is implied by a native build or emulator pass. Before a real
run: coordinate a separate A1200 window, verify live availability and no existing
session owner, and prepare an exact binary-hash/size approval under the user's
physical-testing authorization. Use the supported reviewed no-argument profile-1
runner, with expected line `PT24G VIEW PASS`, bounded output and one-use run ID.
Do not update AmiConnect, retarget its MCP connection, evict a browser owner, or
retry an uncertain launch. Recover the same run ID and explicitly release access.
AmiConnect's fixed profile-1 executable name is `IDSCoreTest`; this diagnostic
makes no IDS acceptance claim.
