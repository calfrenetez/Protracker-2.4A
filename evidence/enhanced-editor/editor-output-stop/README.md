# Editor output-stop binding

2026-09-24. Three editor host sanitizer groups pass (17.755s); pinned native
build passes. Real queued editor source bound to amigus_session demonstrates
edit/Undo/Stop request reset when no consumer lease remains but odd tail exists.
Pending reset prevents session detach. Binding is single-owner and one-shot;
repeated Stop does not call twice. Natural end retains binding for later Stop,
and editor disposal invokes its output callback.

No emulator execution of this new binding yet. Previous session-only emulator
proof remains separate. No native PLAY/backend or physical card enabled. Caller
must poll session cleanup and confirm detach; callback alone does not prove silence.
Native full build includes unrelated display edits and is not UI/release proof.
