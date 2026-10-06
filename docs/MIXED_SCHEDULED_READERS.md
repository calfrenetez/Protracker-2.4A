# Typed paired scheduled reader ownership

`mixed_scheduled_readers` is an opt-in software foundation for one queued Paula
and AmiGUS batch. Its typed queue, keys, domains, events and receipts are separate
from Paula ABI2 and the immediate mixed-owner callbacks. It adds no editor/song
adapter, native PLAY, UI, display, event-loop or production output wiring.

One fixed queue allocation holds at most two command batches, sixteen actions
per batch and thirty-two reader histories. Each action has a unique `(route,
slot)`: Paula slots are 0..3 and AmiGUS slots 0..15. A key includes the complete
queue, fresh session, generation, original trigger ticket, action, route, slot,
owner and serial. Admission preserves strict original frame order. The common
grid maps the original frame to its immutable half-open absolute clock window.
There is no jitter allowance, deadline rebasing, catch-up or automatic retry.

The backend declares both routes, atomic whole-batch publication, independent
command/reader retirement and original-key validation at activation. One
publication call reads the original clock once and makes at most one whole-batch
submit. Two separate route submissions or immediate callbacks do not satisfy
the contract. Confirmed submit refusal retains unpublished work and permits a
later explicit call only before the original window. Every valid clock
observation participates in regression detection, including refused attempts.
Accepted and uncertain submissions preserve their actual possible references
even if callback reentry latches failure. Flags declare a contract; they do not
qualify a backend's atomicity, timing or physical behavior.

Paula TRIGGER geometry describes signed 8-bit CPU cache data and the existing
word, period and volume semantics. AmiGUS TRIGGER geometry uses
`pt_amigus_voice_plan`: enabled software mask `0x800F`, rate 1..`0x40000000`,
aligned numeric start/loop/end pointers and half-open bounds inside the declared
logical 8/16-bit cache. Format and endian flags must match its resource identity.
Padded full capacity has a separate checked card-address bound. Numeric card
addresses are never CPU protection spans. CONTROL changes only period/volume
or rate/gains; its sample geometry is zero. STOP has entirely zero geometry.

Each TRIGGER transfers a distinct mutable reader holder. Shared immutable
masters or caches require independent genuine pins for every holder. CPU span
declarations cover complete master/cache capacities and supporting controls;
card declarations retain reservation/cache identity, version, serial, format,
logical length and full capacity. Upload temporary-pin completion does not end
a playback reader's persistent pins. The core checks declarations and lifetime
rules but cannot prove real pin provenance from a caller record. A production
genuine-holder adapter remains unfinished.

Command detachment and exact per-reader retirement are separate proofs.
`COMMAND_DETACHED` releases only the command domain. A reader releases only after
its exact `READER_RETIRE_PROOF` and zero command references, in either proof
order. An issued command with unadopted readers retains their pins and gives no
key, CONTROL or STOP authority. Positive actual ADOPTED+ACTIVE reader state and
original trigger timing grant that authority. Uncertainty, stale keys and
partial or unknown classifications do not invent activity or quiet. An exact
independent quiet proof can drain its corresponding domain after failure;
invalid classification gives its holder terminal `valid=0` and publishes no
external receipt. Explicit NULL-output services support these later drains.

Construction requires genuine zeroed ordinary scratch of the queried size and
alignment. Complete supplied capacity, config/output, allocator/backend context
extents and control budget are admitted before any write or callback. The scratch
and original config may expire after construction; copied callback contexts
remain alive through close. Full input declarations and CPU span vectors are
validated and copied into private fixed admission scratch before callbacks.
Complete outputs, sources and mutable controls remain disjoint. Known allocation
aliases are never released as fresh ownership. Refusal preserves external
inputs, owners, outputs and registrations; private admission scratch contents
are unspecified after an attempt. Admitted reentry latches failure without
converting actual effect replies into invented release evidence.

Every explicit command or reader service performs at most one corresponding
backend callback. Local `stop` drops only unpublished work and invokes no backend
poll or musical STOP. Submitted or uncertain domains retain all pins until their
distinct proofs drain; `close` refuses while they remain. Final ordinary release
consumes the caller slot before the release callback. Reentry can make `close`
return 0 after the slot became NULL; do not retry freed storage. A later NULL-slot
close avoids expired controls. There is no forced release, rebuild or timer
opening/service.

Saved HOST_ONLY ASan/UBSan qualification is `attempt-oxjh_ms9`: four
compile/run calls pass with zero return codes and empty stderr. Its 923 byte- and
mode-bound inputs are committed `c6783d79ea679bdc8acff3d38ee1c66f1564c9ed` plus
exactly four new overlays and the generated font. It uses genuine 8/16/24-bit
master pins, selective Chip caches, and the existing AmiGUS reservation, cache
and upload software model. Independent literal clock/conversion oracles, exact
master-preserving saves and inherited Paula ABI2 regressions pass. New assertions
cover twelve sixteen-action lifetimes, twenty constructor refusals, 180 typed
admissions, 78 callback/effect/proof cases, 24 explicit clock/refusal/pending/
cancellation groups, six malformed-reader envelopes, eighteen endian/loop/
padding groups and six full alias/capacity/replacement/CONTROL/STOP groups.
Actual nested callbacks, refused full-holder output aliases, consumed hooks and
the close-0/NULL case are asserted. Program markers say SOFTWARE_ONLY; the saved
runner scope is HOST_ONLY. See the [saved host evidence](../evidence/enhanced-editor/mixed-scheduled-readers/README.md)
and independent final source/fixture/custody review. This does not imply native execution.

Production genuine-holder adapters, paired song/editor lowering, a real atomic
backend and native UI/PLAY integration remain later work. Device capacity,
upload ordering/completion, actual all-voice stop, MMIO/DMA, placement, aggregate
stack, task/IRQ timing, exact activation on hardware, physical audio and listening
remain separate open gates. The separate [portable build packet](../evidence/enhanced-editor/mixed-scheduled-readers-portable/README.md)
records compiler/link PASS: 35 calls, 26 ordered translation units and a 187,332-byte
candidate (SHA-256 `8ea8dae1da8ba6f4bfc4e0e64adc7a550dd903c2baf09f37eb956933c3dd3e05`).
That candidate was never executed; native entry and constructor remain NOT_RUN. No native or physical acceptance follows
from this host foundation.
