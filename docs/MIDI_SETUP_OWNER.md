# MIDI-only HOST setup owner

This component is a serialized MIDI sidecar for ownership qualification. It
requires `PT_MIDI_SETUP_OWNER_HOST_ONLY=1` and rejects native targets. The gate
remains intentional: this is not native application integration, a document or
replacement owner, a frontend, or a default activation path. It does not open
CAMD, send messages, change routes, stop audio or replace the editor barrier.

The original caller-owned sidecar allocates one durable setup-controller workspace
through the genuine original editor allocator. Its complete capacity and the
genuinely known original allocator/context extent must remain separate from the
workspace and editor. Context may hold pointers to these objects; it cannot
contain them. Ordinary workspace ownership is separate from the controller's
finite metadata-record budget. Fresh living aligned disjoint allocations remain
an allocator contract; known spans are not a transitive heap census.

Construction captures selected source/journal/allocator metadata before the
callback and rechecks it afterward. A NULL allocation ordinarily returns capacity
with no child and the owned prefix zero. A known alias, unaligned or uncertain
result is retained HELD before raw initialization or release. The entire original
domain stays alive; no forced recovery or arbitrary callback-mutation rollback is
provided. Only the sidecar prefix is initialized; declared tail capacity remains
protected and unchanged.

One modal ticket exposes the original durable controller. Matching return cancels
only the genuine draft, preserving committed and undone redo records. An actual
controller HELD state may end the UI borrow without clearing its uncertainty.
Tickets are not independent release authority or lifetime pins. Serials do not
wrap within one original owner lifetime; all copies must end before owner release
or reinitialization. A new owner at reused addresses may repeat serial1, so no
global ABA or stale-lifetime authentication is claimed. Ticket/status buffers must
also avoid retained records and otherwise unknown caller storage.

The sidecar calls genuine controller close. IN_USE and HELD retain ownership;
neither clears history or accepts a supplied closed/quiet flag. Real editor
disposal must release the journal, including undone redo, before positive close.
Only then may the exact still-owned workspace be zeroed and genuinely initialized
against a final-address initialized editor using the same allocator, or freed
once through the captured original allocator. CLOSED observation and release do
not dereference the former editor/project. Release consumes workspace ownership
before callback reentry. Original context remains alive through records and the
release return. No document is copied, published or replaced by these operations.

The private clean-pair HOST run passed eight genuine groups: six completed rigs
and two whole globally rooted HELD rigs. They cover constructor refusal/NULL,
modal identity/draft return, real MIDI/note chronology and retained redo, actual
dispose/positive close/rebind, CLOSED operations after source destruction,
allocator/release callback reentry, workspace versus record budget, known output
aliases, and retained workspace/controller alias faults. The two retained cases
are whole contexts, not zero-ledger cleanup or two individually exclusive blocks.

Its complete original clean56ff editor C/H provenance remains historical. The
separate saved precompile observation showed all 73 original bodies equal Git
commit `1427d6380d7bda92463bc943f8875d8047fd7d44`; equality is not another HOST
run or native acceptance. The compact actual result is
`evidence/midi-setup-owner/host-private-clean.json`.

The canonical wrapper also passed with the complete preserved working editor
pair: three test methods contain six pure selector cases and one eight-group C
fixture. It admits exactly a complete known clean or working editor pair by hash
and length, rejecting mixed or unknown halves before compilation. Its
32-unit/76-body recipe retains ASan/UBSan, UNDEBUG, the HOST-only definition and
original 68-entry preservation check. The separate actual working result is
`evidence/midi-setup-owner/host-working-pair.json`. The historical private clean
receipt remains unchanged.
No HOST pass establishes document publication, native ABI/link/runtime, frontend,
output quiet, emulator, physical A1200/AmiGUS, timing, listening or release
acceptance. Existing source/context lifetime and genuine caller-placement
preconditions remain required.
