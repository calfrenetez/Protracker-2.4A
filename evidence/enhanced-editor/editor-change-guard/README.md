# Editor mutation/disposal guard

Editor project mutation calls now invoke an optional synchronous owner hook before
changing data. Native imports/bounce/Stop/new/load and disposal call the same hook.
Navigation does not trigger it. Owners must install an idempotent session-closing
callback; no Studio session/device backend is enabled by this change.

The behavioral fixture checks pre-mutation note contents in the hook, undo,
navigation and disposal ordering. Existing editor tests run with the hook unset.
Initial fixture setup omitted its document allocator; fixed before final checks.

Unrelated prepared-display changes remain unstaged. Shared editor/native files
were staged using HEAD-derived patches containing only these guard changes.
Native build includes unrelated display work and is not UI/release qualification.
No emulator, physical hardware or audio-device output testing in this milestone.
