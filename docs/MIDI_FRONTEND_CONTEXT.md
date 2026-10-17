# MIDI frontend context composition

This is an explicit HOST-only composition candidate. It adds no shipping
frontend, native Task or window enrollment. The existing owner, requester,
editor and default build bodies are unchanged.

The caller supplies separate, zeroed, aligned and stationary context and owner
storage, the original initialized editor and the complete original allocator
context. The adapter calls the genuine owner constructor to allocate its
workspace. The original allocator must return fresh disjoint allocations; known
span checks do not discover every transitive allocation or arbitrary alias.

A modal interval borrows the actual owner ticket and embedded requester model
at their original addresses. Apply can consume the model while the ticket still
requires one matched return. Committed and undone metadata records keep the
original controller alive after the requester closes.

Explicit shutdown calls genuine editor disposal, controller closure and workspace
release in that order, once each. The caller first completes its existing
playback, cache, voice and recording shutdown. The adapter supplies no Stop or
hardware quiet. It never frees the caller's sidecar/context, document or UI.
Those owning lifecycles retain their existing release responsibilities.

Uncertain construction, modal return or disposal latches terminal HELD and retains
the original domain without rollback, reset or retry. Ordinary negative admission
remains observational. The portable object cannot park a native Task or preserve
a returned stack frame; the original caller must keep its storage and call chain
alive. CLOSED is a local result, not permission to release unrelated resources.

## Observed HOST behavior

The canonical qualifier returned RC0: three methods, six pure editor-pair
selection cases and one genuine nine-group fixture. Five groups complete and
four retain their original fault rigs until the HOST process exits. The strict
ASan/UBSan build uses 34 translation units and 80 exact source bodies, selecting
the complete working editor pair. View bodies are unselected and preserved.
Compile and fixture diagnostics are empty; the fixture emits its exact 143-byte
oracle. The clean pair passed selector admission only.

The fixture exercises real construction and NULL allocation, known-span refusal,
one failed-open ticket return, Apply/Cancel, journal Undo/Redo and save roundtrip,
callback reentry, disposal veto, original-owner HELD admission and local CLOSED
queries after former sources end. The new HELD admission branch is exercised
through begin; the corresponding disposal branch is source-reviewed only.
Empty sample slots supply no new master PCM or pin-retirement acceptance.

The outer run remains **FAILED**: three newly created C/H/fixture files changed
only file flags and change time between observations. Their contents and all
other recorded facts match. All 80 source bodies, the 68 protected records,
private source copies, saved guard and tools remain unchanged. Full wrapper
metadata at that historical boundary is unconfirmed because no separate raw
after record was retained. The cause is unproven. The run was not repeated or
relabelled. See the [separate functional result and preservation failure](../evidence/midi-frontend-context/functional-host.json).

## Remaining integration

The module excludes native platforms and the existing Task-probe selectors.
Native caller construction, parent input isolation, retained-stack handling and
window execution remain unqualified. No CAMD, device, musical timing or listening
acceptance follows from the HOST test.

Raw frontend New/Load still cannot adopt this owner by adding a guard. Integration
must use the existing typed close/publication/final-address rebind lifecycle and
qualify it separately. The [native document RAM test](MIDI_REQUESTER_DOCUMENT_TASK.md)
has its own independent source, build and execution boundaries.
