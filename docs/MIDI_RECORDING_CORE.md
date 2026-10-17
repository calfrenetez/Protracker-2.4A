# MIDI ownership and quantised recording cores

These are portable, allocation-free C components with an injected message sink
and a sequencer-position interface. They do not open CAMD, send to real devices,
or prove external MIDI timing/audio. The native editor does not yet invoke them.

## Output ownership policy

Each of the sixteen tracker tracks owns at most one active MIDI note. Binding
selects a stable endpoint index and MIDI channel 1–16. The surrounding route
layer must enforce exclusive P/A/M routing and release MIDI ownership on a route
change, mute or stop. A replacement note releases the track's prior note first.
Explicit OFF calls the track release operation; zero velocity also means release.

Tracks sharing the same endpoint, MIDI channel and pitch co-own a single wire
note. A second owner does not retrigger or alter its velocity. Only the final
owner sends Note Off. Different pitches can sound together on that MIDI channel.
For independent articulation/velocity, use different MIDI channels. Controllers,
program and bend affect the whole external MIDI channel, including other tracks
sharing it; this is not per-track isolation of an external synthesizer.

The sink must accept each complete 1–3 byte message atomically or report that no
bytes were accepted. A failed release preserves ownership and the old binding.
If release succeeds but replacement Note On fails, the old note stays released
and the track owns no phantom note. Callers must report the failure, not mark it
played. Calls must be serialized with replay.

Disconnect queues owned note releases and clears active track ownership. Reconnect
flushes remembered Note Offs and sustain releases before allowing new notes.
Partial failure retains unfinished releases for retry, including a pedal-only
failure. Endpoints have stable identities: an adapter must not reuse an index for
a different physical destination while old releases remain pending. An absent
endpoint can leave the external synth sounding until communication is restored;
the software retains that cleanup obligation rather than claiming success.

Stop releases owned notes and sustain that this instance enabled. No broadcast
All Notes Off or All Sound Off is emitted. Program Change, controllers 0–119,
14-bit pitch bend and optional Clock/Start/Continue/Stop bytes are encoded. This
is not an effect translator or a 24 PPQN scheduler; the shared sequencer still has
to supply timing and meaningful effect mappings. Sample-offset effects must not
be repurposed as arbitrary MIDI commands.

## Recording policy

The recorder receives monotonic Q16 sequencer-row positions. It does not infer
rows from wall-clock milliseconds, so tempo changes and input-latency compensation
remain the sequencer/input adapter's responsibility. The implemented grid is
1–64 whole rows; half-grid ties round forward. An explicit logical row limit
prevents silently wrapping past available song storage.

Hold Record waits for an accepted playable Note On. Orphan releases and failed
stores do not start it. Optional velocity and instrument are stored in normal
project events. A key release becomes explicit OFF. A release quantised onto its
own Note On moves to the next grid row, preserving both events and giving a
minimum one-grid duration. Two input events that still need the same cell report
a collision; the core never silently overwrites an accepted event. Releases from
an older replaced key cannot cut the current monophonic note.

Stop records OFF for held keys before disarming. If a store fails or the song has
no room for a release, the affected ownership remains armed and retryable.
Explicit cancel drops recording ownership only; callers must separately stop
output/audition ownership. It must not be used to pretend external notes stopped.

`pt_record_pattern_store` maps logical rows through the project's order list and
uses the ordinary pattern undo journal. It currently accepts MIDI-routed tracks
only. Existing note data is protected, including when an order reuses a pattern.
Existing effect/instrument data is retained unless the input supplies an
instrument; incoming velocity replaces prior velocity and slice selection clears.
Recording across row 63 stores the OFF in the next ordered pattern. Every accepted
input is undoable; grouping a complete take into one command remains future UI work.

## Complete channel-message recording adapter

`pt_record_channel_message` accepts an already complete MIDI 1.0 channel message,
an explicit target track (0–15), independent input-channel filter (1–16) and the
caller's authoritative Q16 sequencer row position. The filter is unrelated to
the project's saved MIDI output channel. Cn/Dn messages have two bytes; other
80..EF channel messages have three, following the
[MIDI Association channel-message table](https://midi.org/expanded-midi-1-0-messages-list).
There is no serial/running-status or System/SysEx parser and no timestamp or
latency conversion.

The adapter validates the full message before filtering and copies its bytes to
scalars before a recorder mutation. Invalid input and valid unselected/non-note
channel messages leave recorder/store state unchanged. Selected Note On/Off
uses genuine `pt_record_input`; zero-velocity Note On follows its Off path.
Quantisation, short-note Off, collisions, stale releases and store/undo behavior
are unchanged. Delegated orphan releases or failures may advance `last_position`;
only adapter-rejected/filtered calls promise no recorder mutation.

This is per-call routing, not automatic key/track allocation. Matching releases
must reach their original recorder track, or the caller must use the genuine
recording lifecycle. Changing track/filter does not release an old key or prove
external silence. The adapter never arms, force-starts, stops, cancels or rearms
recording and emits no MIDI. Hold Record starts only after a real store succeeds.

One [focused private HOST qualifier](../evidence/midi-record-channel/host-private.json)
passed six genuine groups using eight translation
units and sixteen literal project bodies, assertions, ASan/UBSan and warnings as
errors. The exact 155-byte oracle matched, with empty compiler/fixture diagnostics;
38 saved input records, READY, 68 protected records and guard match before/after.
It exercises real validated mixed-route projects, the original pattern store and
journal, Hold Record, row63→64/velocity-zero Off, save/decode/identical re-encode,
current 24-bit PCM roundtrip, Undo/Redo/saved revisions, filters/invalid bytes, all
sixteen input channels, stale releases/collisions, route/occupied-store refusal,
monotonic/end-of-song capacity and explicit track binding. No replacement store
or native input/resource receipt is used. This is HOST software evidence only.
The [canonical wrapper source bridge](../evidence/midi-record-channel/canonical-source-bridge.json)
records its sole repository-root binding change; that normalized wrapper has
**NOT RUN** and does not replace the actual private qualifier's receipt.

A separate [native one-object check](../evidence/midi-record-channel/native-object.json)
passes five stages with RC0 and empty diagnostics using the genuine 68000,
soft-float/nix20 profile. Its 468-byte HUNK_UNIT exports the adapter and retains
`_pt_record_input` as its sole unresolved reference. Actual dependency and
ordinary-preprocessor sets agree: six project bodies and seven genuine SDK
headers, within the unchanged 72-header allowance. All 115 input full-fact
records, READY, 68 protected records and guard match before/after. These are
finite input comparisons: the 115 rows did not directly include the preservation
helper's baseline source. The [separate control-scope clarification](../evidence/midi-record-channel/preservation-control-limit.json)
records that limit without changing the original receipt or inferring retrospective
baseline immutability. The emitted validation/scalar delegation and unchanged Q16
argument were reviewed. One
48-byte compiler stack row is local evidence only; linked recorder/store ABI,
whole-Task stack and native runtime remain unqualified. No executable was linked.
CAMD input, Amiberry, physical MIDI, output timing and listening are **NOT RUN**.

## Optional editor enrollment

[MIDI_EDITOR_RECORDING.md](MIDI_EDITOR_RECORDING.md) describes the separate,
allocation-free stopped-take owner. Actual clean and working editor HOST qualifiers
cover genuine recording/store/history, editor vetoes, master preservation and safe
matched detach. It does not activate native input, recording UI or MIDI output.

## Verification and remaining integration

Host ASan/UBSan tests exercise shared-note co-ownership, note replacement ordering,
release/send failure, endpoint loss/reconnect, sustain-only cleanup, MIDI byte
encoding and 100 lifecycle cycles. Recording tests cover Hold Record, staging,
quantisation ties, short-note OFF, collisions, stale releases, monotonic positions,
last-row boundaries, full-song limits, retryable partial Stop, effect preservation,
and undo/redo of recorded project events.

[MIDI_ENDPOINT_CONTROL.md](MIDI_ENDPOINT_CONTROL.md) describes the portable stable
endpoint registry and control-only CAMD resource owner. Their HOST fixtures and
68000 object checks do not qualify a native CAMD provider or delivery backend.

Native tests use a recording message sink, not CAMD or physical MIDI. Still open:
authorized receiver/source binding, serial/running-status parsing if needed,
automatic key/track allocation, timestamp-to-sequencer conversion, native CAMD
discovery/link and input/output delivery integration, MIDI setup/recording UI
activation, live audition, effect mapping, shared sequencer timing/clock,
transport wiring, real external MIDI and human timing feel. Saved-settings
controller/model/window source exists separately; it does not activate MIDI.
