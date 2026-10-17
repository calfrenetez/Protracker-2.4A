# MIDI endpoint registry and CAMD control owner

The portable endpoint registry maps up to sixteen exact, case-sensitive endpoint
names to stable MIDI-core indices. Discovery and selection are silent. A missing
endpoint retains its pending Note Off and sustain releases; rediscovery never
connects or flushes it implicitly. Reconnect is explicit, and a failed partial
release retains the remaining genuine core obligations. Stop can still release
healthy destinations while another endpoint is unavailable. An identity cannot
be reused while a track or unfinished release still depends on it.

The separate CAMD control owner uses injected resource callbacks. It validates a
complete bounded snapshot of at most sixteen clusters before publishing names,
and marks availability from receiver-list presence rather than its own sender.
It retains the original node, sender, lock and library handles and their borrowed
names. A teardown callback distinguishes confirmed consumption, confirmed refusal
before consumption and an unknown result. Unknown results retain the owner and
its context in a held state; consumed handles are never replayed. Closing takes
at most one child callback per step, in sender, node, library order.

The control owner deliberately has a rejecting MIDI sink. It cannot connect an
endpoint for delivery or emit messages. Both components require serialized calls
and stable original storage; copying a live owner or modifying its embedded MIDI
core independently is outside the contract. Resource teardown does not establish
external-device silence.

The registry HOST fixture covers eleven cases with the genuine MIDI core. The
control HOST fixture covers eight groups using original model library, node,
link and list resources, including four retained unknown-result contexts. The
wrappers copy their exact literal project closure and run with assertions,
ASan, UBSan and warnings treated as errors. These tests cover portable ownership
and control behavior. Separate 68000 object compilation checks the selected source for that
compiler target; it does not link or run a CAMD provider.

Native CAMD activation, installed-library compatibility, discovery and delivery,
the MIDI setup page, frontend/transport wiring and exact musical scheduling still
need integration and runtime qualification. No Amiberry CAMD runtime, real MIDI
device, hardware quiet, output timing or human listening acceptance follows from
these HOST or object-only checks. Opening CAMD may activate a driver and must be
an explicit operation, not a passive audit. This work changes neither the chosen
exact musical schedule nor the exclusive Paula/AmiGUS/MIDI route model.

See [MIDI_RECORDING_CORE.md](MIDI_RECORDING_CORE.md) for note co-ownership,
recording and the complete-message sink contract.
