# EFx clock and cursor core — dev89

The new invert_loop core implements the pinned ProTracker2.3F EFx speed
accumulator and byte-address progression without writing project PCM. It is a
prerequisite for renderer integration; EFx remains refused by the renderer.

The source rules are vendor/pt23f/replayer/PT2.3F_replay_cia.s mt_FunkIt,
mt_UpdateFunk and mt_FunkTable, also present in PT2.3F.s InvertLoop. A nonzero
speed adds its table value to an eight-bit accumulator. Bit7 causes a reset
to zero (the remainder is discarded), then advances the cursor before the byte
is inverted. End wraps to loop start. With no sample bound, a threshold still
resets the accumulator but cannot emit a mutation. EF0 disables updates without
clearing the residual accumulator. Sample binding resets the cursor but retains
speed and accumulator; offsets use a separate bound flag so offset0 is valid.

The API emits only a bounded frame index. Integration must invert the private
mono8 sample value as -1-value, preserving shared mutations between channels
using that sample. A private workspace must survive sample reloads but be
fresh for each render/stem/bounce, and must never alias project PCM. The caller
must follow the replay's exact fresh-row/effect-pass ordering; this module does
not schedule those calls. Rendering support will require actual pinned replay
byte-mutation captures, shared-sample ordering, cancellation and repeatability
checks before lifting the refusal.

Host regression derives intervals independently from the pinned assembly table
and compares 512 updates at all sixteen speeds. It also checks silent/unbound
updates, EF0/resume, range rebinding, invalid input preservation and unchanged
output index when no event occurs. These are source-derived algorithm checks,
not emulator traces, analogue audio or physical acceptance.

Validation: all 95 host tests passed in 144.769 seconds; targeted sanitized
clock test passed in 0.119 seconds. PTInvertTest cross-build passed (16,500
bytes); current build input hashes verified. No emulator window or physical
Amiga was accessed for this milestone. Evidence: evidence/enhanced-editor/dev89.
