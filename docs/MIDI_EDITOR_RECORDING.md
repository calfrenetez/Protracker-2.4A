# Editor MIDI recording enrollment

`editor_midi_recording` optionally binds one stopped, serialized metadata take
to the genuine recorder, complete-channel adapter, project store and chronological
undo journal. It supplies the existing editor change barrier and recording query;
no source/device, scheduler, MIDI output or native frontend is activated.

The caller supplies separate aligned zero storage with its truthful full capacity,
a stationary original editor/project/history and the complete original sampler
allocator-context extent. Known owner/editor/project/master/history/context aliases
refuse before writes. Other opaque/transitive storage and same-address lifetime
identity remain caller obligations. Messages and authoritative Q16 positions are
supplied synchronously; no input pointer is retained. One MIDI-routed target track
and input-channel filter stay fixed independently of later cursor selection or
saved MIDI output-channel settings.

Begin requires the existing callback and context slots to be empty and software
playback/workflow idle. Armed Hold, failed Stop and the stopped-before-detach
interval veto ordinary edits, Undo, settings Apply and disposal. Both callbacks
are inert: they never implicitly Stop, cancel or free a performance. Read-only
usage scans/navigation and deferred Apply intent may coexist; actual transaction
publication remains gated. Genuine recorded revisions stale earlier previews.
Direct APIs bypassing editor admission remain forbidden while enrolled.

Stop records genuine Off events before disarming; capacity/store failures retain
obligations. Explicit cancel drops local recording state and preserves accepted
notes; it does not stop external MIDI/audio or prove device quiet. Neither
operation detaches. Matched detach requires disarmed/no active notes and clears
only this take's two original callback/context slots. Listed identity, allocator,
journal or hook drift latches HELD and retains the whole original domain without
forced cleanup, retry or reset. After successful detach, get reads only owner-local
state and may outlive the former editor/project/allocator context. All aliases must
end before workspace reuse. Existing per-event undo chronology is unchanged.

The [actual dual HOST qualifier](../evidence/editor-midi-recording/host-private.json)
passed once for each complete known clean and working editor C/H pair, using
35 translation units and 82 literal project bodies per variant. Each runs the same
seven genuine groups: five complete rigs and two deliberately retained HELD rigs.
Both exact 163-byte oracles and empty compiler/fixture diagnostics passed with
assertions, warnings as errors and ASan/UBSan. The 258 recorded input full facts,
READY, strict 68 protected records and saved guard matched at every boundary;
the actual preservation baseline source was directly bound.

Checks include original 24-bit master version/pointer/format/descriptor/PCM-byte
witnesses; genuine save/decode/re-encode and Undo/Redo; Hold/short Off across
row63/64; fixed routing, rejection and known-output alias preservation; failed
Stop/local cancel; actual settings-resource eviction invoking allocator-release
reentry; completed usage preview staled by real Stop Off and refused by post-detach
Apply without publication; and detached get after freeing the former source context.
The two fault rigs stay rooted; no cleanup claim applies to them. The
[canonical source bridge](../evidence/editor-midi-recording/canonical-source-bridge.json)
records the wrapper's sole root-binding normalization; that normalized wrapper
has not itself run.

A separate [native one-object check](../evidence/editor-midi-recording/native-object.json)
passed five stages with RC0 and empty diagnostics using genuine 68000/soft-float/
nix20 compilation. Its 4,180-byte HUNK_UNIT retains eleven unresolved references;
editor/view/fixture translation units were not compiled. Actual dependency and
ordinary-preprocessor sets agree: 35 project bodies plus 20 genuine SDK headers,
within the 72-header allowance. All 139 recorded input full facts, READY, strict68
and saved guard match; the preservation baseline source was bound. Individual
compiler stack rows reach 88 bytes and give no whole-Task or callback bound.

Link/runtime, GUI input isolation, playback recording, CAMD/source binding,
timestamp conversion, device/output timing, Amiberry, physical MIDI and listening
remain unqualified. Software admission is not hardware quiet. No default app or
frontend enrollment is included.
