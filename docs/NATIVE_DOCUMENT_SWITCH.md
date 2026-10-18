# Detached native document preparation

`editor_document_switch` prepares a genuine New or encoded project Load in a
separate stationary document. The current document, editor, sample masters and
history remain owned by the original caller. Cancellation checked-releases
only the candidate. Uncertain callbacks or release retain both domains in
HELD and prohibit reset, retry or freeing the control storage.

The caller supplies complete, truthful control/document/editor/allocator-context
capacities and preserves their lifetimes. Preparation excludes concurrent edits
and navigation. Known spans are checked; fresh disjoint allocator results and
opaque child resources remain caller contracts, not a heap-capacity census.
Document/editor snapshots are comparison bytes and are never additional owners.

Default/native publication returns `STOP_UNQUALIFIED`. Preparation and
cancellation are separate from the guarded HOST commit described below.
The native frontend remains unselected: checked native playback stop is still
required before replacing the existing New/Load branches.

`python3 -B tests/test_native_document_switch.py` passes nine genuine HOST groups
with ASan/UBSan: five complete domains and four deliberately retained domains.
They exercise 24-bit sample/save preservation, undo/redo, allocation failures,
capacity/alias refusals, reentry and real document-release quarantine. Leak
sanitizer was unavailable and disabled. Retained HOST cases do not prove native
Task retention or reclamation. See the original compact
[HOST result](../evidence/native-editor-build/document-switch-preparation-host.json).

## Checked HOST commit and native refusal

`pt_native_document_switch_commit_paula` is an explicit HOST-only composition.
It verifies the genuine typed software owner, stops and retires that owner,
checked-disposes the old editor/document, transfers the prepared candidate once,
and initializes the new editor at the final original address. It preserves the
original independent source/song allocation policy. Copied owner structures
supply no authority.

A refusal before irreversible release preserves the old document, history and
masters. After release begins, an uncertain later step retains the remaining
ownership in HELD. The caller must preserve the stationary control and retained
domains; reset, retry and rollback of already released state are prohibited.

`python3 -B tests/test_native_document_switch_commit.py` passes thirteen genuine
HOST groups with ASan/UBSan: six complete domains and seven retained domains.
Together with the unchanged original nine groups, these cover repeated New and
encoded-Load cycles, real history and undo/redo, exact 24-bit master/save bytes,
allocator accounting, final-address rebind, reentry and owner/alias refusals.
Leak sanitizer was unavailable and disabled. See
[the audited HOST results](../evidence/native-editor-build/document-switch-commit-host22.json).

The explicit `PT_NATIVE_DOCUMENT_SWITCH_COMMIT_HOST_ONLY=1` selection hard-errors
for native/m68k/Amiga targets. With that macro absent, both affected native C
units compile with genuine headers and the ordinary APIs; neither HOST-only
commit nor attachment query is present. A separate native preprocessing check
confirms the expected hard error. These two objects are not linked or executed;
they establish no full-editor rebuild or hardware-stop authority. See
[the native object results](../evidence/native-editor-build/document-switch-default-native-objects.json).

Actual checked native stop and frontend publication remain unqualified. The
separate standalone preparation/cancellation Task probe does not exercise this
commit API. GUI/ENV integration, physical A1200, device quiet, timing and
listening are separate gates.
