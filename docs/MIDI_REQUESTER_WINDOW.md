# MIDI saved-settings window source

`src/native/midi_requester.c` is an unbound native window candidate. It supplies
one chooser and one field's bounded editing through the existing requester model.
The visible notice says saved settings only. Adding this source does not add a
menu action, native frontend admission, CAMD/provider connection, route change,
clock/MIDI traffic, automatic Stop or output scheduling.

`pt_native_midi_requester_model_window(parent, model)` uses the caller's genuine
already-open chooser at its original address. It neither copies, reopens nor
resets it. Admission observes the current controller through real get/view calls
before creating UI resources. Refused admission or unavailable GUI leaves the
external model unchanged. Apply/Cancel may consume the model; a genuine enclosing
owner ticket remains the caller's responsibility until its matched once-only
modal return. The original `pt_native_midi_requester(parent, setup, track)` wrapper
retains its own local-model open/cleanup behavior through the same window loop.

Only the window's own font/window and messages are owned here. Original model,
owner, editor, project/history/records, allocator and parent Window/Screen/RPort/
UserPort storage must remain live, stationary and disjoint under their complete
contracts. A normal serialized Task must isolate/drain/restore parent input and
redraw. The window never services the editor timer or parent dispatch. HELD or
uncertain ownership retains the whole original domain; it grants no history
clearing, controller close, workspace release or retry/reset authority. Parent
restoration and durable native New/Load/exit lifetime handling remain unfinished.

One exact private native object qualification passed five commands with empty
diagnostics using the genuine 68000/soft-float/nix20 archive-header/clib profile
and the recorded clean editor header. Its 104 actual dependencies were 32 project
bodies and 72 SDK headers; the optional owner header and all owner selectors were
unselected. The 4,892-byte HUNK_UNIT has both public entries and 28 unresolved
imports. It is unlinked. Its largest individual stack row is 2,260 bytes, which is
not a whole-Task/GUI stack bound. See [the compact object receipt](../evidence/midi-requester-window/native-object.json).

The [one-object source recipe](../tools/probe_recipes/midi_requester_window_object.json)
is declarative. It supplies no executable builder/default target or toolchain/SDK
admission. The existing Task-probe recipe's future header pin follows this added
declaration; its historical object/link/HOST receipts retain their actual earlier
header bindings and are not relabelled. The header's default/HOST/native-owner
gates and model body remain exact; the three affected HOST wrappers change only
the header hash. The frozen adoption proposal records its earlier **NOT RUN**
boundary. Three separate canonical HOST regressions now pass with the complete
working editor pair/quartet: 11 test methods, 38 pure selection cases, four
ASan/UBSan builds and five fixture runs with 773 exact oracle bytes and empty
compiler/fixture diagnostics. All eleven saved input facts and the protected
68-entry set were unchanged. These runs exclude the native window C; they qualify
the model/header and optional HOST owner/document composition only. See
[the current HOST receipt](../evidence/midi-requester-window/host-working.json).

Link, GUI/IDCMP/RAWKEY/VANILLAKEY/rendering, parent isolation/restoration, native
owner/frontend enrollment, repeated document transitions, emulator and real-A1200
acceptance are **NOT RUN** for this window. No device quiet, CAMD delivery, exact
live timing, physical audio or listening qualification is inferred.
