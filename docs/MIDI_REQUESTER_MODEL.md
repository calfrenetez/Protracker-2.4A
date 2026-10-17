# MIDI saved-settings requester model

The portable requester model borrows the existing durable MIDI setup controller.
It supplies a chooser, captured-track output selection, one field's bounded name
editing or clock flags, focus/hit handling, Apply and Cancel. It is not bound to
the native application. The separate [window source](MIDI_REQUESTER_WINDOW.md)
is present but unbound; its exact object-only check does not establish native
owner/frontend composition or a visible MIDI setup page.

Choosing Input, Output or Clock begins one genuine controller draft. Apply commits
only that field through the existing chronological editor journal, then consumes
the model. Cancel discards only its draft. There is no global multi-field Apply,
automatic Stop, route change, CAMD discovery/open/link, MIDI message, input
recording, clock traffic or scheduling activation. Names preserve exact bytes up
to 63 plus NUL; empty clears the saved identity. Input sources and output receiver
inventories are not fabricated or conflated. Any future visible UI must state
that these are saved settings only while native activation is absent.

The model is a separately owned serialized Task object, disjoint from the complete
controller capacity and original editor, project/document/PCM, history, retained
record and allocator/context storage. The opaque minimum-prefix check does not
admit arbitrary model aliases. Only this modal model owns its draft; never copy,
reinitialize or replace a live model/controller/editor/document/history. The model
does not own or free its controller. The original controller and allocator context
must survive all retained journal records after the UI closes. A HELD result may
close model storage while retaining the entire original unresolved owner; it does
not authorize history clearing, teardown or recovery.

The corrected revision3 private HOST fixture passed eight groups using a complete
clean Git editor C/H pair: observational chooser/no-op; focus/hits/cancellation;
bounded text and empty clear; one captured output preserving routes/inactive
slots; one clock transaction; active/veto/allocation refusal; stale/reentrant
callbacks; and retained HELD ownership. Its model is a separate zeroed allocation,
with a real full context/model disjointness assertion. Normal teardown cancels,
actually disposes the editor/journal and closes the controller before freeing the
model. The static HELD rig retains the model pointer and original context.

The earlier revision2 run really passed, but its embedded model overlapped its
allocator/guard context and did **not** qualify the expanded placement contract.
Its source/control/streams remain historical and unchanged. The separate compact
revision3 result is `evidence/midi-requester/host-private-clean-r3.json`.

The canonical wrapper accepts exactly either known complete clean or complete
working editor C/H pair by both hash and byte count, refusing mixed or unknown
pairs before compilation. Its actual working-pair run passed three unittest
methods: six pure selector cases and one eight-group C fixture. That separate
result records the selected complete pair in
`evidence/midi-requester/host-working-pair.json`. The historical private clean
receipt's successor NOT_RUN field describes its earlier boundary; it is unchanged.
The strict recipe uses 32 translation units, 76 literal bodies, ASan/UBSan,
UNDEBUG and the original 68-entry preservation check.

No HOST result establishes native ABI/link/runtime, window/IDCMP/keyboard/rendering,
parent-input isolation, durable native document-transition integration, emulator,
physical A1200/AmiGUS, hardware quiet, listening or release acceptance. The 8-bit,
16-bit and 24-bit master/cache architecture and exact scheduling remain governed
by their own implementation and qualification boundaries.

The optional HOST-only [owner/modal composition](MIDI_REQUESTER_MODAL.md) uses the
genuine original owner ticket around this same model. Its private clean pair and
separate real document-admission companion passed first; four separate canonical
working-pair/quartet HOST qualifiers now pass as well. The original model wrapper
now records77 literal bodies because the conditional quoted owner header belongs
to its static graph. Its32-unit no-HOST-flag recipe and original eight-group
fixture/oracle remain unchanged and passed separately with the complete working
pair; see `evidence/midi-requester-modal/host-canonical-original-model-regression.json`.
This does not relabel the historical76-body receipt above or activate a native
requester. Earlier private receipt NOT_RUN fields retain their original boundary.
