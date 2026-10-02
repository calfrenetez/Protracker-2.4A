# Incremental mixed-backend capability preparation

`mixed_preflight` checks one shared Paula/AmiGUS sequence before promoting any
master or allocating/uploading a playback copy. The supported geometry, global
tempo/effect flow, backend rules and strict playback deadline policy are unchanged.

The serialized owner uses `pt_mixed_preflight_begin`, `step`, `take` and `close`.
Begin validates the project and static metadata, copies options, capabilities and
format, and owns two bounded allocations: analysis workspace and sequence. These
initial validation/scans remain synchronous and outside playback deadlines.

Each later step does exactly one analysis phase:

- At most 256 timeline measurement ticks.
- One interval `next` operation.
- At most 256 consumed frames.
- One completed command plan and its Paula/AmiGUS capability checks.

The analysis makes no master pins, derived copies, uploads, voice callbacks or
PCM output. It accumulates source masks privately; pending/failure reports expose
zero masks. A successful complete traversal publishes both backend masks and
preserves global diagnostic action/channel indices. A late refusal publishes no
partial success and cannot transfer its sequence.

Take rewinds and transfers the same audited sequence once, without allocation or
remeasurement. The caller owns that sequence afterwards. Close releases untaken
sequence/workspace storage at any phase and is idempotent. Project arrays and PCM
remain borrowed and immutable throughout analysis and transferred playback;
callbacks must not edit or reenter. The synchronous compatibility wrapper drives
these same bounded steps until completion, so it remains synchronous as a whole.

`pt_mixed_owner_prepare` now advances one analysis phase per call before any
source promotion. Only complete successful analysis allows the existing bounded
master-promotion phase. Stale-source failure and owner/editor cancellation close
the analysis before the existing reader/quiescence barriers; uncertain barriers
continue retaining the enclosing owner and engine tokens. The native editor
advancement seam can interleave calls while preparation remains unfinished.

Host sanitizer fixtures cover synchronous/incremental report parity, progress
bounds, hidden partial masks, late backend refusal, one-time sequence transfer,
allocation failures and cancellation during measurement, traversal and completed
untaken analysis. Mixed owner/editor tests cover cancellation and stale-source
refusal before promotion, pending closure barriers and existing transport behavior.
Progress bounds are not a wall-clock guarantee. Native timing, physical capacity,
transfer completion, voice-stop and output/listening acceptance remain separate.
