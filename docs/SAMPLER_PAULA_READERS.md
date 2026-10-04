# Genuine sampler binding for split reader/command ownership

`sampler_paula_readers.c/.h` is a separate opt-in task-side binding for
`scheduled_readers`. Existing future, lineage-v1, flags7 and all-reader retirement
APIs are unchanged. This binding uses real immutable sampler versions and a
separate optional selective signed8 Chip representation cache; it does not install
or qualify a native activation backend.

The pool has independent bounded command and persistent-reader handle capacities
(1..8 each), a control-allocation budget and a Chip-cache budget. Initial project
validation and bounded control allocation are synchronous. Each preparation step
performs one transition/action, copies at most 4096 master bytes, or converts/copies
at most 256 Chip bytes. A TRIGGER retains one genuine master pin and one exact cache
lease. It preserves authoritative 8/16/24-bit mono/stereo data and low bits; stereo
requires an explicit selected channel. No implicit rate conversion or downmix is
added. Cache geometry and format follow the existing Paula representation contract.

A command holder owns only small command/request/batch metadata. Each TRIGGER has
an independent persistent-reader holder containing its genuine pin/cache lease,
original numeric source/route and geometry, monotone holder token and immutable
original trigger/action identity. Its full sample/cache capacities are registered
as reader-resource spans; mutable reader and command holder extents are separate.
Every held version/backing, marker and cache allocation stays alive until its exact
reader-domain release callback. The pool keeps caller-owned original source spans
for output protection even after promotion or invalidation.

CONTROL/STOP requests contain only a by-value full original key and numeric
track/sample/channel. The key must come from the adapter's positive reader-key
getter, which forwards the actual caller output to the core guard. It records a
key only after actual ADOPTED+ACTIVE qualification. Begin, preparation and enqueue
first resolve that exact cached full key to a registered LIVE reader, then recheck
the core's actual original-key permission and exact current version/lease. Forged
session, serial, owner, trigger, action, slot or generation refuses before holder
callbacks. There is no raw predecessor pointer, source descriptor, master pin or
cache lease in a CONTROL/STOP preparation or READY holder. A predecessor may retire,
be replaced, change version or close while a command is prepared; enqueue re-resolves
the copied key and refuses safely. Matching cache address/serial alone never grants
original-key permission. The core's monotone same-slot replacement and STOP rules
remain authoritative.

| Proof or operation | Small command holder | Persistent reader and master/cache |
| --- | --- | --- |
| Prepare/cancel before transfer | Local holder can close | New untransferred trigger resources cancel; controls own none |
| Successful enqueue | Transfers independent command control | Each new trigger transfers its independent genuine reader control |
| Actual command DETACHED | Retires command; explicit close permits slot reuse | Retains all reader storage |
| Actual reader RETIRED while commands refer | Command ownership unchanged | Core retains the reader owner until last command detach |
| Reader RETIRED with zero referring commands | Command ownership unchanged | Exactly-once release unpins the cache and immutable master |
| Unknown/malformed/partial ownership report | Retains possible ownership | Retains possible ownership; independently exact domain proof may release with invalid terminal classification |

After a trigger transfers, retrieve its independent reader handle before closing
the detached command. Its original key and frame persist across command slot reuse.
The command's reader-handle getter verifies live registration plus token before
reading an origin pointer, so a recycled reader address cannot revive an old handle.
Retired reader handles remain registered until explicit close; LIVE close refuses.
Pool close refuses every registered handle and never force-unpins or automatically
cancels submitted work. Retired resources can still close after source/generation
invalidation. A forbidden callback reentry during positive retirement defers resource
release to explicit reader close; it never discards the confirmed retirement.

All adapter-known outputs and input declarations protect full master capacities,
original project metadata, sampler versions/jobs, cache storage, pool and handle
controls before mutation. Guard scans metadata only. Invalid aliases leave output
and state unchanged. Caller must additionally keep arguments disjoint from opaque
queue/backend/allocator/Chip callback contexts whose full extent the adapter cannot
know. Public actual reader-key output goes through both adapter and core guards.
Contexts and original borrowed storage stay alive through pool close. Calls are
serialized task work; callbacks forbid reentry or editing borrowed inputs. Only
designated release callbacks release their corresponding independently retired
holder's resources. No owner is copied/cast from a legacy exclusive owner.

The shared host fixture uses genuine sampler promotion/cache operations with host
allocator injections. Its bounded backend model represents positive clock/key/
adoption/detachment/retirement receipts in serialized task work; it is not a native
IRQ implementation. Six precision/channel cases retain the full original masters,
promoted version-header before-images and exact single cache pin through 20 CONTROLs with two
command slots, then separate STOP/reader retirement and command detachment. Further
cases cover 52 preparation cancellation points, allocation/budget pressure, cancelled
controls without extra pins, retirement before/after READY, replacement retirement
before stale admission, source-version change, real reader-address reuse with fresh
token, forged keys, full-capacity/header/backend output aliases, uncertainty/wrong
retirement context/invalid terminal classification, and finite callback reentry.
Each fixture ends with zero owned allocations and budgets. Host tests do not prove
physical Fast/Chip classification, installed backend reference transfer, hardware
voice-stop, actual scheduling, DMA/IRQ/audio or human listening acceptance.

## Current validation status

The frozen shared fixture passes one ASan/UBSan host group with 21 full dependency
checks and one actual execution. Its cache-byte comparison uses the existing public
playback packer as the parity oracle. Original master PCM and promoted version headers
are checked before and after ownership transitions; the fixture does not snapshot
every owned version PCM byte or explicitly assert adopted-table/pin-job aliases.
The source guards cover these known spans, with those test limits kept explicit.

One pinned 68k portability build passes 32 command records and produces a
157,632-byte HUNK with active SDK assertions. It remains native NOT RUN. No installed
backend, actual device-domain transfer, Fast/Chip memory-class qualification,
IRQ/DMA, timing, audio or listening acceptance follows.
See [saved host/compiler records](../evidence/enhanced-editor/sampler-paula-readers/README.md).
