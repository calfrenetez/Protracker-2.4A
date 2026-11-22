# Mixed Paula, AmiGUS and MIDI sequence

A Wavetable project can now produce Paula, AmiGUS and MIDI actions from one
software timeline. Each track keeps its assigned route and the existing Paula
map limits playback to four hardware voices. The routed preflight checks the
audio actions against the existing Paula and AmiGUS backend rules. It returns
the same rewound sequence for playback preparation. The ordinary audio render
and Paula editor playback entry points retain their existing behavior.

MIDI actions carry the project track, note and velocity. The sequence shares
position, break, loop, delay and tempo changes across all routes. Lookahead
prepares a common batch before its exact frame. The consumer must commit at that
frame and release all owned voices and notes on stop. Endpoint resolution and
delivery belong to the consumer. Unsupported MIDI effects and MIDI clock flags
fail preflight. This work uses the 16-bit render options and leaves the existing
sample masters intact.

The MIDI preparation code converts routed notes to complete messages ahead of
their frame. It binds endpoint names and MIDI channels from the project and
uses the existing note owner to count shared notes. Only a confirmed batch for
the original frame and preparation serial advances the accepted note state.
Cancelling a refused batch leaves accepted notes intact, so Stop can release
exactly the notes that were accepted. The existing CAMD interface still needs
a delivery backend. CAMD's documented timestamps apply to received messages
and do not schedule outgoing messages.
[CAMD documentation](https://wiki.amigaos.net/wiki/Camd_Library#MIDI_Timestamps)

The timed MIDI owner derives the same half-open counter window as the audio
queue from the shared epoch, rate and counter frequency. It submits the whole
prepared batch before the window and waits for the provider's exact session,
generation, frame and serial receipt. An uncertain or incomplete receipt
retains the batch. A proven early or late issue retains the actual notes for
Stop and reports a timing failure. The software fixture uses a provider model.
This timed interface has yet to be bound to a native CAMD provider or to the
combined Paula/AmiGUS delivery path.

The AmiGUS Mini RAM adapter follows the per-longword pacing read used by
[ProTracker 4.1.2.1](https://github.com/akustikrausch/protracker4/blob/v4.1.2.1/src/sys/amigus41.asm#L478-L505).
The existing 2.4G C adapter reads FIFO status after each RAM data write and
latches failure if the FIFO is full. The transfer owner retains responsibility
for cleanup. Credit to the ProTracker developers Lars Hamre, CryptoBurners,
Tom Beyer and Oliver Ickler, and to Andreas Wendorf for the 4.1 additions.
The integration uses the register-access technique in 2.4G's C code. The
[PT4 licence](https://github.com/akustikrausch/protracker4/blob/v4.1.2.1/LICENSE)
describes conditions for direct source reuse.

The 48 kHz host fixture produces both audio attacks and the first MIDI attack at
frame 0. It checks a delayed MIDI attack at frame 6080 after a tempo change,
note release at frame 10080, MIDI note zero at frame 12480 and final release at
the shared F00 boundary at frame 14880. A fake MIDI sink checks exact bytes.
The preflight fixture checks Paula and AmiGUS source masks, missing MIDI route
configuration and a late AmiGUS sample refusal. The synthetic register test
checks the FIFO fault and upload refusal.

The Amiga cross-build covers the routed fixture and the changed native RAM
adapter. The editor's mixed Play path, native scheduled output, CAMD delivery
and physical A1200 timing and listening checks remain open. The
[exact native scheduling decision](NATIVE_TIMING_DECISION.md) governs that work.

The routed MIDI fixture has a logs-only Amiberry runner in
`tools/test_routed_midi_emulator.py`. Use `--fixture wire` for the render
fixture and `--fixture schedule` for the timed provider model. It takes the
exact cross-built executable and SHA-256, the current profile SHA-256, the
emulator and DevBench PIDs and a new evidence directory. Run it using the
shared harness Python, its scripts on `PYTHONPATH` and the lease for the
currently admitted 030 guest. The runner
checks the guest's CPU and the staged bytes before execution. It preserves the
output and verifies the guest exit code before removing its staged files.
Starting, stopping, restoring and releasing the guest remain the session
owner's responsibility. A pass qualifies the core assertions in Amiberry.

Both MIDI fixtures also completed in the shared Amiberry 68030 profile using
a private DOS-only disk with the guest bridge disabled. The guest copied back
the exact executables, returned zero, and produced both complete assertion
logs. The owned guest stopped normally. The shared files were restored and the
reservation was released.
[Offline 030 test receipt](test-evidence/routed_midi_offline_amiberry_20261122.json)
