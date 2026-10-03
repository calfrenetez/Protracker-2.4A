# Scheduled output ownership contract

The user selected preservation of the exact musical boundary. The portable
`scheduled_output` seam defines future publication and reader ownership; it does
not implement or qualify a timer interrupt, audio.device timestamp command,
Paula activation, physical timing or listening. Existing classic and enhanced
song/voice paths are unchanged. The current immediate native adapter has no
compatible timestamped API and cannot be used as this backend.

A task first validates the song and prepares its signed8 cache geometry and all
required master/cache/continuing-reader pins. It then gives the queue one uniquely
held owner token plus protected spans covering their full capacities and owner
metadata. The queue does no conversion, acquisition, eviction or sample reads.
Each accepted event copies a complete ordered TRIGGER/CONTROL/STOP batch and
storage-span metadata. Refusal leaves the caller's token, outputs and queue intact.
The copied action address is not itself an ownership proof: the caller's held
owner and current-generation check supply that contract.

Open requires three explicit backend declarations: timestamped future submission,
atomic complete-event publication, and confirmed reader retirement. Missing or
unknown capability bits refuse before allocation. These are adapter promises to
test, never proof that a native implementation meets them. The queue makes one
bounded allocation for at most eight events; each has at most64 actions and128
protected spans. Full queues refuse rather than overwrite live event storage.

The original epoch, counter frequency, logical rate and generation are immutable.
Existing `elapsed_clock` arithmetic computes each admission interval as
`[epoch + ceil(frame*frequency/rate), epoch + ceil((frame+1)*frequency/rate))`.
The task can publish only before that interval, using an actual backend clock
read. The backend must independently validate its live clock/generation at atomic
publication and activate at the target boundary. The task never calls ordinary
voice start/control functions at the deadline, passes a predicted timestamp as
an observation, or substitutes an immediate-start fallback.

Events are enqueued and explicitly submitted in increasing frame order. A
confirmed refusal retains no backend references and leaves the event queued for
an explicit retry. Any other submission result may retain the immutable event;
it latches failure, blocks further publication, and retains its pins. Poll or
cancel must acknowledge that **all future activation, DMA readers and callbacks**
have retired before the owner is released. Command acceptance or initial activation
alone cannot retire a trigger's cache. Out-of-order retirement is allowed because
each ticket retains independent storage. Ticket serials never wrap/reuse.

Executed retirement receipts carry actual observation and command-issue counters.
Both must be ordered within the original frame interval. A late, incomplete or
failed receipt latches failure; if reader retirement is confirmed, storage can
still be released safely. These reported counters are not physical first-sample
measurements. Pending/error acknowledgements retain storage indefinitely. Stop
closes publication, releases unsubmitted owners, and makes at most one cancellation
attempt per submitted ticket per call. Close refuses while any owner remains.
There is no automatic device reset, force release, retry or exit on uncertainty.

All queue operations are serialized task work. Callbacks are bounded and may not
reenter or edit inputs. Backend-owned task/interrupt exclusion, activation code,
hardware capability checks, actual clock binding and worst-case execution bounds
remain to be implemented and separately qualified. No queue operation is an IRQ
entry point. Protected-span guards inspect metadata only, including missing
positive storage and address-wrap refusal; they never read capacity padding.
The backend declares its complete mutable context extent so output pointers cannot
corrupt its clock or publication state. Allocator contexts retain their existing
caller-side disjointness requirement; unknown context sizes are not guessed.

The existing `pt_paula_prepared` object cannot be copied into these future slots:
it captures exclusive live voice/map state and currently transfers leases at
immediate apply. A subsequent song/backend integration must produce independently
held future owners and synchronize publication/cancellation. This seam does not
change that lifecycle, extend current native scheduling capability, or clear the
preserved failed first-boundary and CIA timing results.
