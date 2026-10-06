# Checked mixed playback lifetime in the actual editor

The optional `pt_editor_mixed_checked_begin` adapter adopts the existing checked
owner before its first allocator callback. It uses this editor's sampler,
project and revision, after pre-borrow master establishment has been stopped and
the engines bound. The complete external checked control, editor, binding
(including its publisher) and optional composite context span are guarded before
returned child storage is initialized. The existing public established-owner
constructor retains its original admission rules and three-extra-span capacity.
The private publisher-container constructor admits only the exact contained
publisher, with two remaining extra spans. Default legacy linkage remains usable
without the new adapter module.

Stop/edit/dispose first cancels preparation, then confirms transport alarm,
readers and counter closure, owner closure, and finally zero-child checked-control
finish. Every refusal keeps the finalizer and contexts adopted and vetoes actual
mutation/disposal. Confirmed closure clears adoption; completed 8/16/24-bit masters
stay sampler-owned and exact master-preserving save remains valid. No promotion
occurs during the borrow. Unlisted opaque contexts must remain independently
nonoverlapping and all borrowed controls must survive until confirmed stop.

One retained ASan/UBSan run passed three groups/six compile-run calls:

- 39 actual establishment/editor/checked-owner lifecycle cases across 8/16/24
  bits, 18 whole-control allocator-alias/public-admission cases, and 15 legacy
  editor transport regressions.
- 27 prior editor establishment cancellation/reentry/save/master-retention cases,
  24 control-alias cases and 15 legacy editor regressions.
- The original 66 established-owner and 255 mixed-owner scenarios.

All six calls returned zero; stderr was empty and 905 source identities stayed
exact. The tests use the indexed editor baseline with these nine own source/test
changes, preserving the 16 unrelated worktree paths. The first compilation failure
is retained separately: the fixture incorrectly assigned a pointer to an unsigned
synthetic interrupt flag. No executable ran on that attempt. Only that fixture
source changed for the passing run. The Python unittest entrypoint was not itself
executed here; the retained runner used its equivalent sealed C source closure.

The pinned portable m68000/softfloat compile/link passed 89 calls and sealed 228
dependencies. `PTEditorCheckedTest` is 378988 bytes, SHA256
`f37fef467d74e4684a6663350f25fd8ba7c30f8e43056e48896351af6faf2469`.
It has never been staged or executed on a target. Emulator and physical execution
are **NOT_RUN**. Injected software clock/voice quiet predicates do not qualify real
DMA, card RAM, hardware voice stop, IRQ behavior, exact live musical timing,
native memory placement, total stack, audio or human listening. The native PLAY
and UI/event-loop binding remain unfinished; synchronous bridge binding is
separate and exact musical scheduling is unchanged.

`verify_saved.py` audits saved bytes and results only. It never compiles, launches
an executable, reserves a target, or contacts transport. The root source review
is a self-review; no independent reviewer is claimed. Native/physical validation
needs fresh shared ownership and exact-candidate guards after the other task's
current held session is independently recovered and released.
