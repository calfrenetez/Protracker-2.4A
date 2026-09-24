# Sampler generation / song ownership bridge

The bridge captures slot master generations, owns provider context and constructs
bindings for the Studio song controller. Explicit stop closes core playback before
edits or owner replacement. Defensive generation/table checks refuse stale pulls.
In-place pattern mutation is not intercepted; native editor actions remain unwired.

Three sanitizer-enabled host fixtures passed: sampler Studio ownership, new sampler
song lifecycle, and core Studio song controller. Coverage includes budgeted true24
promotion, stop/edit/restart, stale-generation refusal and stop-before-owner release.
PTSamplerSongTest is included in the native cross-build.

No emulator or physical run in this milestone. Existing guest evidence for the
core song controller does not qualify this new bridge. Native build includes
unrelated display work; no UI/release or audio-device acceptance is claimed.
