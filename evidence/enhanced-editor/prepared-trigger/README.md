# Bounded trigger preparation — 27 September 2026

Song batches calculate one trigger command per preparation call after uploading
all selected cache leases. Commit uses the command only for the exact private
action, matching rate/format and captured address/length. All baseline source,
master, cache, whole-batch and stop checks remain. Public manual dispatch and
range restoration are unchanged. The fixed command bank shares the bounded
song allocation; cancellation invalidates it with its owning leases.

Host dispatch ASan/UBSan PASS (9.264s). Its converter wrapper proves at most one
conversion per preparation call, three prepared commands, no recalculation on
repeated readiness, and three retained whole-batch conversions at commit instead
of six total. Tests cover8/16-bit derived plans, duplicate sample leases, stale
master refusal and cancellation. Staged editor lifetime suite PASS (30.267s).
Native cross-build and editor-main syntax PASS.172 staged inputs/font and binary
were verified. Candidate407772 bytes SHA256
e7e764e1620bfaf62f41b608529e616278c181761b6e12274a9d54ac61f52dcc.

Shared030 run render-files-1790507664608561000 PASS within the separately
coordinated120second cumulative test window, RC0.1241 tracked Fast allocations,
zero owned bytes, no Chip fallback. All timers/watchdogs and leases closed; four
DMA channels off. Exact run/launcher cleanup plus independent absence verified.
Explicit shared030/DevBench release sent to AmiConnect; no recovery hold.

The earlier candidate's90second timeout and subsequent natural RC0 completion
remain preserved in ../prepared-trigger-timeout/, including guarded recovery.
The revised candidate removes only a newly introduced redundant command-copy
source check immediately after mandatory source_location; no original check was
removed. The process budget was raised for this growing full suite only, not as
a playback lateness tolerance, and the failed run was not relabeled a pass.

Native observed startup costs:10258ticks (one voice) and91949ticks (16 voices)
at709379Hz. These include instrumentation and emulator scheduling and vary across
runs; they do not establish a speedup or realtime budget. Whole-batch capability,
control and restoration lowering remain synchronous. No audio/card/physical
operations or native enhanced PLAY enable. Masters/persistence/layout unchanged.
