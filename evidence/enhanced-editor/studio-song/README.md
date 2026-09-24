# Owned Studio song controller

The controller owns copied master bindings, bounded output/plan buffers, sequence
and Studio mixer. Open preflights before playback; three bounded allocations are
released correctly when any startup stage fails. Natural end, stop and internal
failure release sequence/mixer state and all pins. Close frees the controller.

Each pull performs one transition or one <=256-frame block. Pre-roll yields
progress without output. Project/source lifetime is borrowed and immutable; this
does not enable edits during playback. Session output is borrowed scratch.

Targeted host checks cover mixer, command dispatch, song controller, interval
reader and incremental sequence. Song output matches reference at1/17/256 frames;
allocation-stage refusal, pinned stop, natural cleanup and sticky provider failure
are tested. PTStudioSongTest is included in the Amiga cross-build.

No emulator/physical execution or live-output claim. Native build includes unrelated
uncommitted display changes and does not qualify the editor UI/release. Editor
lifecycle integration, device queues and AmiGUS transport remain unfinished.
