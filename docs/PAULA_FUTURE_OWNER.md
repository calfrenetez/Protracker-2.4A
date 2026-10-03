# Independently held future Paula trigger sources

`sampler_paula_future` supplies a task-side resource bridge to the portable
`scheduled_output` contract. It does not integrate the song producer, existing
`pt_paula_prepared` or exclusive `pt_paula_voices` lifecycle, native output,
interrupts, Paula DMA, AmiGUS, or hardware timestamps. Backend flags remain
adapter promises; no host test qualifies actual activation or audio.

A pool owns its own sampler-Paula representation bridge. A caller policy bounds
it to one through eight owner handles, a control-storage byte budget covering the
pool and each separately allocated holder, and a dedicated optional Chip copy
budget. Master storage remains authoritative, charged to the original sampler
budget. Eight/16/24-bit mono/stereo masters retain their precision; the caller
explicitly chooses the channel of the signed8, even-byte padded derived copy.
Allocation, initial validation, metadata checks and eviction remain synchronous.
They have no latency guarantee. A preparation step performs one transition, at
most 4096 master bytes, or at most 256 Chip bytes. No partial view is published.

Each owner independently retains its genuine master versions and cache leases.
Tokens monotonically increase, never reuse, and refuse at overflow. An entire
batch has at most four sources/actions and one action per stable slot from
`pt_channels_paula_map`. `enqueue` checks the batch's original generation/frame,
actual current cache lease/range, aligned signed8 pointer/word extent and stable
slot. Protected master or version-header addresses cannot substitute for a
trigger cache. A failed enqueue leaves the holder, batch, ticket and queue
unchanged. Ownership transfers only when the queue itself returns OK. Publish
rechecks source versions, routes, project header and full trigger geometry.
In-place PCM/marker edits are forbidden, not rescanned. Sampler edits and undo
change the generation/current version and refuse stale publication.

Only TRIGGER is supported. CONTROL and STOP refuse as a whole batch, including
when all cache refs are held. An accepted or retired event does not prove active
playback. Active-reader control/stop needs a later typed publication/activation
lineage contract and remains unfinished.

A retained retrigger request may refer to a registered transferred-live earlier
owner of the same queue/generation and stable track/slot. It holds an internal
preparation borrow, retains a new genuine master reference and acquires an exact
HIT lease (same slot/serial), then severs the predecessor dependency before
ready. If the predecessor retires while borrowed, its original resources remain
held until preparation obtains those independent refs or cancels. Its opaque
handle cannot close while borrowed. Once ready, either event can retire without
releasing the other event's refs. Retrigger is a new explicit activation request,
not inferred continuation of an active hardware voice.

Untransferred pending/ready holders can cancel or close. Transferred-live
holders refuse both; only queue positive all-reader retirement releases refs.
Uncertain submit/poll/cancel keeps all live storage. The pool refuses close until
every owner handle has closed; it never forces unpins. A retirement reentry
violation defers resource release rather than losing retirement or freeing a
borrowed source. The allocator/backends and all pool/queue operations must still
honor the serialized, non-reentrant contracts.

Public outputs protect the complete pool, all registered owner/job storage,
borrowed document/current project/master/marker extents, all held private version/backing
headers and full Chip cache capacities. The internal bounded
`pt_sampler_version_spans` accessor exposes actual metadata-only extents, never
private struct layout or a contiguous extent inferred from accounting bytes.
Zero-length spans are omitted. Caller outputs and input control descriptors must
also remain disjoint from opaque borrowed queue storage and allocator/Chip
callback contexts, whose sizes this API cannot discover. Genuine live handles,
source tables and storage, sampler, project, queue and callback contexts outlive
all owners/pool operations, including stale cancellation. Initial document-backed capacities remain borrowed;
private version addresses are protected only while genuinely held/current, and
are omitted after retirement rather than policing a later unrelated allocation
at the same address. Unpinned pending descriptors are identity snapshots, not
resource ownership. Source replacement may
change the project header, but may not free borrowed originals before close.

The shared host fixture exercises actual sampler pin jobs, Chip upload jobs and
portable queue callbacks with an injected timestamp backend. It checks precision,
channel selection, first bytes and padding; bounded steps and hidden partial
views; independent retirement/retrigger refs; allocation and budget pressure;
source/control aliases; whole-batch refusal; stale generation/routes/source
metadata; and uncertain submit/cancel retention. Such tests prove resource
contracts only. Native compilation, emulator execution, physical memory/DMA,
interrupt timing and human listening are separate unperformed qualifications.

Current qualification update: the three host sanitizer groups and a separate
standalone 68k portability build now pass. The 150,852-byte native fixture has
not run. Compilation does not establish Exec Fast/Chip placement, DMA, native
output or timing. See [the saved host/compiler evidence](../evidence/enhanced-editor/paula-future-trigger-owner/README.md)
and [current architecture status](SAMPLE_MEMORY.md).
