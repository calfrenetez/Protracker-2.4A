# Mixed playback editor lifetime binding

1 October 2026. The zero-initialized editor-thread binding claims the editor's
existing veto-capable change barrier. Both backend engines must use this exact
editor sampler and project; owner preparation uses its allocator. The binding
adopts a caller-owned private transport only if transport begin adopts resources,
including a clocked-begin failure. Invalid start leaves timer cleanup with caller.
While adopted, only binding service/signal/stop APIs control the pump and owner.

STOP, edits, import, undo, replacement and disposal call the same bounded close
barrier. Uncertain alarm IO, backend readers or counter release retain remaining
owners and veto mutation/disposal/detach. The counter remains until alarm and sample
owner confirm release. DONE retains ownership until explicit close. Binding,
editor, project, pump and callback contexts survive refused close; detach precedes
editor reinitialization/free, including after successful disposal. Existing guards
refuse attachment; no other owner's hook is displaced.

New15 host scenarios (5 cases ×8/16/24-bit masters) cover cancellation during
preparation, pending startup, active mixed readers, failed initial clock read and
completed playback. They check wrong sampler/project refusal, invalid/double start,
preparation refusal after start, pending signal forwarding, navigation without
stop, refused pattern/sample/import/undo/dispose/detach, byte-identical enhanced
save during refused mutation, confirmed readers while counter remains, final
release and idempotent detach. Three related ASan/UBSan modules PASS29.806s:
editor_mixed15, editor_paula3 precision lifetime sequences and mixed_owner240.
Indexed editor sources preserve unrelated uncommitted display work.

No native frontend PLAY action, event-loop wait, production output/device/IRQ or
UI is installed by this component. Timer/voice tests are injected. Actual editor
application integration, private native timer wiring, playback timing, sound,
endurance/pressure and physical qualification remain separate requirements.

Indexed treee6a7598f16a6c2cb671b010a5bdd680140b96cc5,176 verified source hashes,
zero generated sources and three verified binaries. PTExecEditorMixedTest254656
bytes SHA2561c7b4e99b5e30d48a72b65ce1ecc2b2fb3435f7a1415565074b164238dec1946.
ONE shared030 run1790839793751141000 PASS11.134s outer, native15 injected lifetime
cases separately from host240 mixed regressions.138 Fast allocations/zero owned
bytes, all Chip/cache/master/reader/lease/pin/binding/transport ownership closed.
Normaldone/rc0/exact path cleanup and independent subsequent locked process/profile/
bridge/68030/running/all4DMAoff/run+launcher absence PASS. Coordinated release sent
after proof. Bounds90fixture/140runner/200outer<=240reservation; no retry/lifecycle/
actual timer/output/UI/MMIO/DMA/audio/physical operations. Native allocator and
editor lifetime proof does not qualify the frontend, hardware or playback timing.
