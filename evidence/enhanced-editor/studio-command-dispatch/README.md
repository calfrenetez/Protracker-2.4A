# Studio command dispatch

The adapter applies explicit audited render commands through Studio's pinned
master provider. Bindings map descriptor identity to key/version. Unknown or
ambiguous sources and invalid channels are refused before callbacks. Any later
failure stops all session voices and releases current/pending pins; callers must
abort and discard command state, not retry a partially applied plan.

Host checks: Studio mixer, tick reader, new 16-channel true24 dispatch comparison,
render command plan, sampler ownership/promotion, and final dispatch recheck.
All passed under the existing sanitizer test wrappers. The native cross-build
includes PTStudioPlanTest. Its old GCC rejected a nested zero initializer in the
initial fixture; explicit initialization fixed this test portability issue.

No emulator/physical execution or live output qualification in this milestone.
The full native build includes unrelated uncommitted display changes, so it is
not an editor release or layout acceptance. Full song scheduling, synchronized
command-state advancement, editor integration and AmiGUS transport remain open.
