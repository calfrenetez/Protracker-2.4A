# Metadata-only bridge binding before checked playback preparation

`pt_editor_mixed_bridges_bind` is an optional callable path for the attached editor
with an already owned, dedicated empty wavetable backend and exclusively idle
voice slots. It initializes both bridges, maps and voice/quiescence callbacks
without the repeated synchronous full-project validations of the ordinary public
bind/sync functions. The pre-borrow master job must already have stopped. The
complete genuine zeroed output is guarded against project/sampler storage,
editor/binding, backend/reservation, input/vector and up to eight named opaque
context extents before any initialization. Unlisted context start pointers inside
output also refuse; unlisted full extents remain independently caller-disjoint.
The supported ABI must let the established owner's four constituent control
spans cover the complete bridge object; host and m68000 compilation enforce it.

No allocator, ownership predicate, device callback, semantic value scan, master
pin/promotion, sample copy, cache acquisition, MMIO or upload occurs at bind.
Refusal preserves output and every input. Success copies function/context values,
with no fallible operation after the publication boundary. This is not a semantic
or backend-ready certificate. This path must be followed by the actual checked
editor owner and its complete cancellable INITIAL validation and two-route audit
before playback. Ordinary public validation/binding and exact scheduling remain
unchanged. The callback contexts, source and output must survive confirmed owner,
voice/quiescence, cache/backend closure; controls cannot be copied or reinitialized.

Three host ASan/UBSan groups/six calls passed with empty stderr and 909 unchanged
source identities. The changed fixture passed nine poisoned-metadata/full checked
validation/ownership/live-drain cases and 75 protected-output/context/cache/
admission refusals across 8/16/24 bits. Event, order and current PCM/marker values
were poisoned during admission; an ownership spy refuses any admission callback.
Invalid musical events still failed the later genuine validation before any
output, and lost ownership still failed the separate live gate. Valid work reached
both injected outputs and retained actual editor edits until both drains and
transport alarm/counter/zero-child closure. Completed masters stayed sampler-owned.

The original 39 checked lifecycle, 18 alias and 15 editor transport cases passed
inside the new fixture and again as a separate standalone fixture. The original
66 checked-owner and 255 owner cases passed separately. The only change to the
old checked test is a default-equivalent wrapper enabling inclusion of its existing
case bodies. The Python unittest entrypoint was not itself invoked; the retained
runner compiled and ran its equivalent sealed C source closure against the indexed
editor baseline, preserving all 16 unrelated worktree paths.

One pinned m68000/softfloat compile/link passed 90 calls with 231 dependencies.
`PTEditorBridgesTest` is 385144 bytes, SHA256
`21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e`.
The first version's nine plus 60 cases and compiler checks passed and remain
preserved privately; the changed version added the conservative pointer and ABI
checks before its own separate first qualification. Neither product has been
staged or executed on any target. Emulator and physical execution are **NOT_RUN**.
These injected software callbacks prove no native placement, aggregate stack,
Chip RAM/DMA, real sample RAM region/capacity/order/completion, voice stop, IRQ,
exact live musical deadlines, audio or human listening acceptance. Native PLAY
and event-loop integration still remain unfinished.

`verify_saved.py` checks retained bytes/results only, with no producer or target
replay. The root source review is a self-review, not an independent reviewer.
Target testing needs the peer's current held connection-stage failure to be
independently recovered and released, then new exact-candidate coordination and
live guards. The peer's EMPTY45 recovery receipt was a historical release; it
never granted root a new target window or made shared transport permanently free.
