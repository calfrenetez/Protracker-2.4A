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
