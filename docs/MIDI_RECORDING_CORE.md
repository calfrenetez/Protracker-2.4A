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

## Verification and remaining integration

Host ASan/UBSan tests exercise shared-note co-ownership, note replacement ordering,
release/send failure, endpoint loss/reconnect, sustain-only cleanup, MIDI byte
encoding and 100 lifecycle cycles. Recording tests cover Hold Record, staging,
quantisation ties, short-note OFF, collisions, stale releases, monotonic positions,
last-row boundaries, full-song limits, retryable partial Stop, effect preservation,
and undo/redo of recorded project events.

Native tests use a recording message sink, not CAMD or physical MIDI. Still open:
CAMD discovery/link lifecycle and stable endpoint mapping, input parsing and
channel/track allocation, the MIDI setup UI, live audition, effect mapping, shared
sequencer timing/clock, transport wiring, real external MIDI and human timing feel.
