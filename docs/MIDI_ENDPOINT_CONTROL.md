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

The classic 68k adapter binds that owner to CAMD's public library vectors. It
copies cluster names and checks receiver presence under the original CAMD lock.
Sender creation checks the cluster before and after linking. The adapter uses
the classic public cluster prefix so receive drivers are counted alongside
application links. [Classic vector table](https://github.com/cnvogelg/amiditools/blob/b45aa4d91815e56f432dbeb5f2ca0e59c63ec379/amiga/fd/camd_lib.fd)

The exact `PTCamdControl` fixture ran on Amiberry 68030 with the library absent
first, then with CAMD 40.5 installed in the private disk. The driver's directory
was empty. The fixture discovered a local receiver, filtered out a sender-only
cluster, and verified a note-on and note-off through the local receiver queue.
The original links, node and library reference closed. [Upstream CAMD 40.5](https://github.com/kmatheussen/camd/tree/55a2f589bba0277bb7e779547a0e6d9a15aff2c0),
[native test receipt](test-evidence/native_camd_control_amiberry_20261123.json)

Opening CAMD can activate drivers. Editor setup and exact scheduled delivery
remain separate integration work. The tested public send call can block on a
full hardware buffer. The combined schedule still needs a provider that can
prequeue work and prove exactly when it issued. [CAMD 40.5 send implementation](https://github.com/kmatheussen/camd/blob/55a2f589bba0277bb7e779547a0e6d9a15aff2c0/src/putmidi.c)

See [MIDI_RECORDING_CORE.md](MIDI_RECORDING_CORE.md) for note co-ownership,
recording and the complete-message sink contract.
