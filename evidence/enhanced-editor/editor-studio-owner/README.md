# Editor Studio owner attachment

Caller-owned attachment installs an idempotent song-closing editor change guard.
Refuses other owners' guards, uses the sampler allocator and detaches only its own
hook. Must detach before editor free/reinit; editor disposal may safely run first.
No native PLAY/device output path is enabled by this helper.

Host fixture runs actual pinned Studio playback, then edits a note, undoes and
disposes the editor; each mutation closes playback. Navigation preserves it.
Final tracked allocations reach zero. Existing editor and guard tests also run.
PTEditorStudioTest is cross-built; no emulator or physical run for this milestone.

Native build includes preserved unrelated prepared-display work and does not
qualify the editor UI/release. Native instantiation and output transport remain open.
