# Private native timer adoption by the editor binding

1 October 2026. The OS2+ native helper opens a private counter and WAITECLOCK
request, then delegates pump adoption to the editor's existing mixed-session
binding. Start submits no alarm. Binding service/signal/stop own the adopted
pump thereafter. Its storage and editor/engine/callback contexts must remain
alive until confirmed stop; direct pump/owner calls are forbidden while adopted.

Invalid arguments leave resources untouched. Counter/device open failures close
partial idle resources. Refused editor attachment closes both idle timer requests
without adopting or freeing its sample owner. A clocked-start failure after pump
adoption retains both timer and editor ownership for bounded explicit close.
Editor change/disposal/detach therefore use the same retained-reader barrier as
normal playback. No global TimerBase, MMIO, IRQ/output or UI installation.

Three related sanitized host modules PASS29.685s: editor_mixed15, editor_paula3
precision lifetime sequences, mixed_owner240. Host excludes OS2+ timer helper;
native request behavior is separately qualified. Indexed exports preserve all
unrelated uncommitted display/classic-layout work.

The native fixture adds9 cases (3×8/16/24-bit masters): a prepared owner primes
and submits a future one-second alarm, proves private pending signal and refused
editor mutation/import/disposal/detach until readers confirm; an unprepared start
fails after adoption and still retains the counter/editor barrier; an invalidated
editor attachment refuses adoption and closes idle requests preserving the owner.
All test paths stop without voice starts and explicitly confirm all timer request,
port, device, counter, owner and binding release. Invalid quantum and repeat start
refuse. Fixture-only bounded Delay(1) permits cancellation completion; production
helper never waits on unfinished IO.

This component is native editor timer wiring, not a frontend PLAY/event-loop or
actual output driver. Pending-alarm cancellation does not prove wake punctuality,
strict musical-boundary success, audio/listening, endurance/pressure or physical
acceptance. Physical hardware remains off and unprobed.

Indexed treea93055c9fc282d1612e9ec01c9172d1d8036d53b:180 verified source hashes,
zero generated sources, three verified binaries. PTExecEditorMixedTest260760 bytes
SHA25616b8ad20eb9110acceb56657386cad332cdd7b5eaef4d2208ec00ff39b2a15bd.
ONE shared030 run1790840198019652000 PASS14.556s outer:24 native scenarios,
15 injected+9 actual private binding cases.210Fast allocations/zeroownedbytes;
every pending request completes/cancels before device/request/port release, and
all sample/cache/master/readers/pins/leases/editor/pump ownership closes. Normal
done/rc0/exact owned cleanup plus independent subsequent locked process/profile/
bridge/68030/running/all4DMAoff/run+launcher absence PASS. Explicit coordinated
release acknowledged by AmiConnect and forwarded to peers after proof. Bounds90fixture/140runner/200outer<=240 reservation.
No voice starts/UI/output/MMIO/DMA/audio/lifecycle/retry/physical operations.
