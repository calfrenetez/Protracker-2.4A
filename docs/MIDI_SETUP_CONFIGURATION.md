# Saved MIDI configuration controller

`src/editor/midi_setup.c` supplies a silent, task-side controller for the existing
project MIDI preferences. It is not yet connected to a visible MIDI setup page.
It does not open CAMD, create links, send messages, record input, produce clock
traffic or activate a playback route. These preferences are **saved configuration
only** until a separately qualified native frontend/provider uses them.

One draft edits the input endpoint identity, one active track's output identity,
or the existing send/receive clock flags. Apply commits exactly that draft as one
resource command in the genuine editor journal. A future dialog must use a
per-field Apply; this API is not an atomic all-fields transaction. Cancel only
discards the draft. Names are exact, case-sensitive bytes, at most 63 plus the
final NUL; an empty identity clears the preference without choosing a replacement.
No endpoint edit changes the exclusive Paula/AmiGUS/MIDI route or MIDI channel.

Copied discovery data distinguishes unconfigured, unknown, missing and present
identities. Present means an inventory name match, not a connection or capability
guarantee. Input inventory must describe clusters with senders; receiver-only
output discovery cannot populate it. Device reorder or disappearance preserves
the configured identity. The existing project format saves input, clock flags
and active track outputs. Inactive slots are preserved in memory and are not
claimed to roundtrip through serialization.

Visible no-op names preserve existing bytes after NUL without invoking a change
hook, allocating a command or changing the dirty revision. Invalid and stale
drafts are also silent. Actual commits require stopped transport, no active
workflow/capture job, positive existing editor change admission and fresh checks
after external recording queries. They never stop a performance implicitly.
Resource undo/redo uses the same chronological journal, independently refuses
active recording and checks the exact scoped before/after field for outside
edits before moving its cursor or revision. Capture/library/backend ownership
still requires the caller's real admission barrier; these checks alone do not
prove external hardware quiet.

Fixed-size journal records use the genuine editor allocator and a separate
explicit metadata byte budget. Undone redo commands remain charged until actual
discard. A fresh command must fit before journal eviction can release an old
one. Allocation failure preserves the document. An allocator result overlapping
known live storage is held without initialization or release; that unresolved
obligation is not reported as exclusively owned record bytes. HELD observation
returns without writing the caller's output. No forced recovery is provided.

Original editor/project/controller storage and the captured allocator context
must stay alive while drafts or retained commands exist. Cancel before document
transitions and drain the genuine journal before destroying the controller or
budget. Close itself does not dereference the former editor/history and refuses
remaining record ownership. Pointer, shape, revision and generation checks detect
ordinary replacement; they do not establish universal same-address lifetime
identity or authorize reinitializing live storage.

The first private HOST qualification passed eleven fixture groups with the
complete clean Git editor C/H pair from `56ff32d0a5c71df094b115d36c8b368030e6f3dc`.
It covered real note/channel/resource chronology, saved revisions, actual project
encode/decode, bounded redo ownership, allocation refusal, callback reentry,
recording-busy undo/redo, stale drafts, exact-field conflicts, directional
inventory, all three routes, inactive slots, 8/16/24-bit PCM value preservation,
ordered disposal and HELD observation. The compact historical result is in
`evidence/midi-setup/host-clean-git.json`.

The original canonical `tests/test_midi_setup.py` pinned a separate complete
working editor C/H pair and the exact 31-unit/74-body source closure. Its eleven
groups passed once with the original strict 68-entry preservation
snapshot and ASan/UBSan policy unchanged. That separate result is recorded in
`evidence/midi-setup/host-working-pair.json`. The historical clean-pair record's
working-pair NOT_RUN field describes its earlier qualification boundary.

These HOST passes do not establish native ABI/link, visible frontend, emulator,
physical A1200/AmiGUS, output scheduling, recording, device capability, listening
or release acceptance.

The successor wrapper now admits exactly either complete known editor C/H
pair using both lengths and hashes before compilation. Its actual HOST run
passed three unittest methods: six pure pair-admission subcases and one real
unchanged eleven-group C fixture using the complete working pair. Both mixed
pairings, an unknown hash and an unknown size refused. The pure clean-pair
admission case is not a new clean C compile; the earlier real clean-pair pass
remains a separate historical receipt. The selected pair, exact source/helper/
wrapper and saved command/result records are bound in
`evidence/midi-setup/host-pair-selector.json`. The strict 68-entry preservation
snapshot and saved guard before/after pins were unchanged. No native/frontend/
CAMD/target or full toolchain qualification is inferred.
