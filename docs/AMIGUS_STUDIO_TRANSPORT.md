# Studio PCM transport review and packing

24 September 2026. Review targets the existing pinned public source commit
`d8c9a0429f41cd5f3dbadae34ef438e45c9c3718`, not an unverified latest version.
The local SDK checkout matches that commit. No card, firmware or driver was touched.

## Supported path found

The public library SFD exposes discovery, reservation and interrupts, not a
high-level streaming submission API. The pinned AHI driver implements separate
24-bit PCM playback; this is independent of the 8/16-bit wavetable engine.

Primary pinned references:
- [Mode table](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/src/amigus_ahi_modes.c): stereo24, six bytes per frame.
- [Register definitions](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/header/amigus_hardware.h): format5, FIFO write offset0x0c, endian/channel flags.
- [Packing function](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/src/copies.c): PlaybackCopy32to24 emits three32-bit words for four samples. Its AHI inputs are left-aligned32; our signed24 masters are right-aligned and must not be truncated again.
- [Worker](https://github.com/necronomfive/AmiGUS-pub/blob/d8c9a0429f41cd5f3dbadae34ef438e45c9c3718/Software/Drivers/AHI4/src/worker.c): underrun recovery uses full playback initialization to restore24-bit stereo alignment.

## Implemented now

`amigus_pcm_pack` independently packs numeric FIFO words from48k stereo24 PCM.
It preserves all24 bits, interleaves L/R, and emits MSB-first words. No native
register access or upstream implementation code is included. Two stereo frames
form three words. A trailing odd frame is retained across blocks; explicit finish
adds at most one silent frame and reports it. No hidden padding between blocks.
Capacity/format refusal preserves state and outputs. Each call handles at most256
frames with caller storage, no allocation and no master edits.

Tests independently decode output bytes against the source at1/17/256-frame
partitions, check fixed extreme-value words, final padding, repeat finish,
capacity refusal and format rejection. Host ASan/UBSan and pinned Amiga compilation
pass. This packer is not connected to the consumer or an actual card yet.

## Remaining before live output

Verify Mini-specific register access widths, FIFO usage units/capacity and safe
interrupt/cancellation sequence against Mini documentation and driver accessors.
Implement bounded FIFO capacity handling and explicit alignment/reset state before
wiring submission. Submission completion must distinguish copied host memory from
still-audible device FIFO contents. Reserve PCM exclusively and unwind ownership
on every failure; do not equate host lease release with device silence.

No emulator positive-card, physical Mini, interrupt ABI, underrun recovery,
bus throughput or68030 real-time deadline is qualified. A24-bit DAC alone would
not prove24-bit source playback; the pinned PCM implementation is the basis here.

## Bounded staging adapter

The pinned AHI interrupt source converts FIFO usage to bytes by multiplying by2;
its hardware header defines2048 playback LONGs. Accessors use16-bit register
reads and32-bit FIFO writes. These are source observations, not Mini bus-cycle
qualification. The Mini register map still needs direct reconciliation before
native MMIO is enabled; no fixed capacity is assumed by the new core.

`amigus_fifo` now stages packed words behind a caller-supplied port. Port capacity
is explicitly normalized to32-bit free words. Poll does at most one capacity
query and one three-word write; less than three free words stalls. Busy submission
preserves the existing block. All writes must have confirmed completion; uncertain
partial writes or capacity faults block further writes until confirmed reset.
A pending/failed reset preserves software state and refuses new audio. Reset also
clears any odd-frame carry, preventing contamination of the next stream.

Its submit/poll/cancel signatures match studio_consumer, but live integration is
not yet wired. Poll completion means copied host data is no longer referenced;
it does NOT mean the device FIFO is empty or silent. After final producer output,
call explicit finish and poll its possible padding, then separately drain/stop
the device. No padding is inserted between blocks. External code must stop the
producer before cancellation and preserve exclusive port ownership.

Host sanitizer tests cover fixed words, stalls, final padding, write uncertainty,
capacity fault, pending/failed reset and successful recovery. Pinned Amiga
compilation passes. No native port, card access, interrupts or performance proof.

## Consumer integration fixture

The real queue/consumer now drives staged packing through a fake FIFO port.
257 stereo frames partitioned1/17/256 produce identical packed bytes plus exactly
one reported final silent frame despite intermittent capacity stalls. A partial
write failure followed by pending/failed reset keeps the queue lease busy until
reset succeeds. All tracked queue allocations are released.

Session orchestration must explicitly reset the FIFO even when the consumer has
no outstanding lease: consumer stop only cancels a transport-held lease. Software
may still retain an odd tail, or hardware may still contain copied audio, after
that lease is gone. The fixture explicitly finishes the tail before normal detach
and explicitly resets after output. Native device drain remains unimplemented.

## Session-level shutdown

`amigus_session` owns the consumer/FIFO orchestration and borrows the queue and
port contexts. Normal completion flushes the odd tail, polls staged words, waits
for a separate device-drain acknowledgement, then requires confirmed disable/reset
before detach. Stop aborts queued audio and requests reset even without a consumer
lease. Reset acknowledgement authorizes consumer lease release; failed/pending
reset keeps the owner attached and blocks reuse. Step retries reset in bounded
calls. Errors remain sticky diagnostics even after cleanup succeeds; DONE phase
and successful detach indicate safe ownership release, not a successful playback.

Failed initial reset leaves a recoverable attached owner. Caller must drive reset
recovery and detach before freeing contexts. Stop the producer separately first.
No callback implementation touches hardware here. Host tests cover normal end,
delayed drain, drain error, held/no-lease Stop, failed initial reset and repeated
reset failure/recovery. Native compilation passes; emulator session execution is
pending. The native port must actually disable playback and reset FIFO, not merely
report that the request was issued. Physical silence/timing remain unqualified.

## Mini map reconciliation and native session evidence

Pinned `Documentation/AmiGUS_mini/AmiGUS_mini_Register_Map.xlsx`, sheet
`Main Register`: F28 documents format101 as24-bit stereo; B47:F47 lists reset
strobe at0x08; B49:F49 lists16-bit data ports0x0c/0x0e; B50:F50 lists pending
FIFO words at0x10. This supports the format and word-unit interpretation, but
not FIFO depth or a promise that one CPU long write is atomic on PCMCIA. Native
access ordering, capacity bound and stop/reset acknowledgement remain to verify.
Source digest/cell evidence: `evidence/enhanced-editor/exec-amigus-session/mini-map-review.json`.

Session fixture now passes shared030 under production allocator: six tracked
Fast allocations, zero final owned bytes and budget refusal. This closes the
session emulator gap, not native port/physical audio acceptance.

## Native adapter review, pinned source

Further inspection of the same pinned SDK gives these implementation constraints:

- `Base/src/amigus_pcmcia.c`, CreateCardPrivate, publishes `agus_PcmBase` from
  its selected card mapping plus PCM offset. Use that returned pointer after
  discovery/reservation. Do not hard-code PCMCIA mapping or switch memory mode.
- `AHI4/src/amigus_pcm.c`, StopAmiGusPcmPlayback, disables sample rate first,
  clears playback IRQ flags/masks, then strobes FIFO reset. Session reset success
  must include equivalent quiescence, not only the strobe write.
- `AHI4/header/amigus_hardware.h` declares8192-byte/4096-word playback capacity,
  rate48k code7 and enable bit0x8000. Treat these as pinned driver assumptions;
  validate Mini applicability before exposing live playback. A conservative
  configurable capacity must never exceed verified device capacity.
- `Base/src/amigus_base.c`, dispatcher around lines363-387, invokes the typed
  `AmiGUS_Interrupt` callback. The header type places context in a0. This supports
  a0 at source level; compiled callback ABI and native interrupt teardown remain
  separate checks before installation.

Editor output-stop binding now passes shared030 with57 tracked Fast allocations,
zero final owned bytes and budget refusal. This verifies abstract shutdown
requests, not native interrupt handling, PCM output or physical silence.

## Injected register-port adapter

`amigus_register_port` now implements the capacity/write/reset/drain interface
through injected read16/write16/write32 operations. No native bus binding or
playback start is supplied. Caller must provide verified capacity in16-bit words
and an exclusive PCM ownership predicate with serialized access. Initialization
performs no I/O. Reset is required before use.

Free32-bit words are floor((capacity_words - pending_words)/2). Impossible usage
faults instead of wrapping. Triplet writes recheck capacity and preserve order at
0x0c; uncertain writes invalidate alignment until reset. Reset performs bounded
disable-rate, clear-playback-IRQ-flags, mask-playback-IRQs, FIFO-strobe writes and
reads rate, mask and usage. Enable clear, playback mask clear and zero usage are
all required before success. Pending readback never grants alignment. Drain only
reports FIFO empty, not DAC/analogue silence. Native readback and bus semantics
must be qualified before this implementation can claim device quiescence.

Five host sanitizer groups pass, covering all seven reset I/O failure points,
partial triplet writes, unknown ownership, odd-unit capacity, impossible usage
and delayed readback. Pinned native compilation passes. No emulator register
fixture execution, card access, playback enable or native interrupt installation.

## Register/session integration model

The integration fixture now connects queue, consumer, packer, FIFO staging,
session and register adapter. Its injected bus models clear-bit IRQ semantics,
16-bit pending-word accounting, explicit reset delays and a write that accepts
part of a triplet before reporting failure. It checks exact packed bytes and
final padding, preservation of capture IRQ bits, waiting until pending words are
zero, no further writes after uncertainty, ownership-loss refusal and cleanup
only after confirmed reset. No hardware enable/playback clock is simulated;
tests explicitly advance the FIFO usage model to represent consumption.

Six AmiGUS host sanitizer groups and the pinned native fixture build pass.
This is a behavioral model, not proof that a Mini implements these readbacks or
bus transactions. Positive card discovery, native reservation/MMIO and real
playback timing remain separate work.

## Normal PCM reservation lifecycle (2026-09-24)

`amigus_reservation` provides a serial, caller-owned library/card lifecycle.
It selects an explicit enumeration index (bounded to 16), rejects cycles,
unsupported cards and missing PCM blocks, and distinguishes missing library,
missing card, PCM busy and other driver errors. It reserves PCM alone and uses
the stable owner address as the reservation identity. Failed acquisition closes
its library reference without releasing somebody else's card. Close releases
its own reservation before closing the library, and is idempotent.

A single access lease refuses close while a downstream output session may still
use the card. The caller must acquire it before port/session setup and end it
only after confirmed reset/detach and interrupt removal. This is an explicit
caller obligation, not a hardware state inferred by the lifecycle. The API must
not be copied, moved or reentered while open. FreeCard returns no status, so
close confirms invocation order, not independently verified driver release.

`src/native/amigus_reservation.c` wraps OpenLibrary/CloseLibrary and the existing
public SFD diagnostic shims with a per-context library base. It neither calls
the adversarial ownership diagnostic nor introduces MMIO, interrupt installation,
FIFO capacity defaults or playback enable. Known card type plus PCM presence
is sufficient for reservation only; it does not verify Studio capabilities.
The native editor does not instantiate this adapter yet.

Validation: seven host sanitizer groups pass. The pinned native compiler links
`PTAmiGusReservationTest` with the production adapter and fake-library fixture.
The fixture does not call production library callbacks. No emulator execution
or real library/card test is claimed for this milestone. Reproduce with
`tools/build_amigus_reservation.py` and AMIGA_CC set to the pinned compiler.
Evidence: `evidence/enhanced-editor/amigus-reservation/`. Next: qualify the
fake-library fixture on the shared030, then integrate access ownership with
session reset/detach tests before considering any native device operations.

Reservation/session integration now passes eight host sanitizer checks and
shared030 execution: failed initial reset, Stop with a held buffer, and natural
odd-frame completion retain library/card ownership through pending/failed reset.
The caller detaches before ending its access lease and releasing the card. Every
fake port callback asserts a live reservation. Three tracked Fast allocations,
zero owned bytes; no real library callbacks/MMIO. The production native adapter
is linked but not invoked. Build with `tools/build_amigus_reservation.py`; run
the coordinated shared harness with `--studio-memory reserved-session`. Evidence:
`evidence/enhanced-editor/exec-amigus-reserved-session/`. Native playback remains
unwired; native absent-library runtime and verified device capabilities remain
separate unfinished work.

The discovery-only native adapter probe now handles unavailable amigus.library
on shared030 (two attempts, library=0, closed=1, rc0). Core discovery bounds
enumeration to16 cards, detects cycles and reports known PCM descriptors without
reserving or exposing card pointers. It closes every successfully opened library
reference. Host absent/empty/unsupported/cycle checks pass; positive library/card
and playback behavior remain untested. A discovered PCM descriptor is not a
verified Studio capability. Evidence: `evidence/enhanced-editor/amigus-discovery/`.

## Explicit resource selection (27 September 2026)

The reservation lifecycle now also accepts an explicitly selected WAVETABLE block.
Its original open entry point and discovery count remain PCM-only; Studio callers
retain the same resource and busy-code behavior. Support/reserve/release callbacks
carry the selected resource, and combined masks are refused to avoid ambiguous
partial acquisition. The separate `amigus_wavetable_cache` owner keeps its access
lease until all cached sample leases retire. This does not enable native Studio
output or bind a wavetable register bus. See `AMIGUS_SAMPLE_RAM.md` for ownership
and the fake-library/injected-bus evidence boundary.


## Native reservation-call ABI (27 September2026)

The production native Find/Reserve/Free callback bodies now have shared030 execution
coverage against a private process-local vector table. Assembly captures verify the
published SFD offsets/registers, full32-bit status, per-context bases and exact PCM/
wavetable owner identity through success and busy unwind. The fixture never installs
or opens a mock library; core open/close are injected callbacks. It uses no MMIO or
interrupts and releases its one Fast allocation. This validates compiler/register
binding against the pinned ABI, not actual library/card compatibility or output.
Evidence: `evidence/enhanced-editor/amigus-native-abi/`.


## Explicit native callback entry

The SDK callback typedef names a0, while its nearby comment names d1. With the
pinned compiler flags, a typedef-declared C callback reads a stack argument.
`src/native/amigus_interrupt_entry.s` explicitly bridges a0 to a normal C handler,
preserves all registers except the d0 result, and returns with RTS. The dispatcher
refuses null/unarmed/missing handlers and normalizes only result1 to handled.
Callers must keep its stable context alive until confirmed interrupt removal;
arming or changing it requires a disabled, quiescent source. It installs nothing.

An independent assembly caller verifies the context and volatile-register
preservation under shared030 execution, alongside the native reservation vectors.
This is ordinary task-context ABI evidence, not actual interrupt installation,
concurrent teardown, MMIO or hardware qualification. Native PLAY remains unwired.
Evidence: `evidence/enhanced-editor/amigus-callback-abi/`.

## Partial interrupt installation ownership

`amigus_interrupt_owner` now guards an existing reservation access lease before
calling install. A nonzero installation error may follow callback retention in
the pinned driver. Failed installation therefore still requires stop, and access
end/card close remain refused. Removal is requested once; only a separate bounded
quiesce callback returning exactly1 releases the guard. Native install/remove
wrappers verify context and call the published -48/-54 vectors without a global
library base. They supply no hardware quiescence implementation.

Host sanitizer and private-vector native fixtures cover successful/partial
installation, full32-bit status, callback entry/data/registers, both resources,
pending/error/invalid quiescence and safe release. Evidence:
`evidence/enhanced-editor/amigus-interrupt-owner/`. This is ordinary task-context
execution of private vectors, not actual interrupt installation/removal or audio
acceptance. Native output remains unwired.

The PCM session also requires exactly1 from its separate drain callback. Unknown
positive status is a fault, not successful completion; guarded reset still must
complete before detach. Host regression and seven-case native fixture pass with
zero retained memory. See `evidence/enhanced-editor/amigus-drain-status/`.

## Explicit polling start boundary (28 September 2026)

The injected register port now offers `pt_amigus_register_start`: after confirmed
reset and at least one complete six-word FIFO prefill, it writes signed MSB-first
stereo24 format5, waits for exact format readback, requests48k playback with
rate0x8007, and waits for exact enabled readback. It does not request interpolation
or playback interrupts. Capture IRQ bits remain untouched. Each poll is bounded;
format/enable writes occur once per reset. Partial or unknown write results,
invalid capacity/alignment, unexpected enabled settings, playback IRQ masks or
lost ownership poison the port until confirmed reset. A pending readback remains
pending; callers must provide their own operation deadline or Stop policy.

`pt_amigus_session_open_started` adds this boundary to the queue/session without
changing the legacy transport initializer. The first complete packed triplet is
copied before start; further FIFO writes wait for exact start acknowledgement.
An empty stream never enables output. A single stereo frame starts only after its
explicit final silent-frame padding. Stop or start failure retains the consumer
and queue until disable/reset is confirmed, including after a possibly effective
enable write. Start and drain contexts must remain alive until detach.

This is an injectable software path, not native device output. The production
editor still uses its existing injected output entry point; no real register bus
is bound. Native Mini capacity, register readback, bus ordering, interrupts,
underrun recovery, physical sound and real-time throughput remain unqualified.

Validation: seven isolated host sanitizer cases pass, including existing editor
and reservation fixtures. Shared030 production-Exec register-session run
1790565288286312000 returned0 and released all12 Fast allocations. Exact packed
bytes, delayed/failed enable, empty/odd tails and Stop while pending pass. All
owned paths were removed and independently checked, DMA off, window released.
Evidence: `evidence/enhanced-editor/studio-start/`. No physical target touched.


The editor output owner now exposes `pt_editor_studio_output_bind_start` while
attached and idle. The optional callback applies to both existing ordinary and
PCM-reserved starts, persists across Stop/restart, and is cleared on successful
detach. Rebinding is refused while a queue or reservation remains owned. Its
context must outlive detach or an idle rebind. An adapter can bind
`pt_amigus_register_start`; no actual native register adapter or PLAY UI action is
introduced. Host coverage includes natural completion, pending-start edit/undo/
Stop/dispose, unknown positive acknowledgement, and reservation retention until
session reset and adapter quiescence both complete.

The isolated host editor fixture and shared030 production-Exec run
1790565586339483000 both pass this editor binding, including disposal during
pending start. Native return0 and zero owned Fast bytes; all DMA off and exact
cleanup independently confirmed before release. Evidence:
`evidence/enhanced-editor/editor-studio-start/`.


### Capacity-bounded startup prefill

`pt_amigus_session_open_prefilled` and the editor's `bind_prefill` accept a
nonzero number of complete two-frame triplets before enable. After confirmed
reset, the target must fit the port's reported empty capacity; oversized targets
and capacity errors retain a failed cleanup owner rather than waiting forever.
The bounded writer accumulates prefill across producer blocks, including one-frame
blocks. A finished stream shorter than the target enables after its available
complete/padded data; an empty stream never enables. Pending acknowledgement still
blocks further writes. Existing start APIs keep their one-triplet compatibility
minimum. Neither that minimum nor any chosen target proves sustainable throughput;
a native adapter must select its buffering policy from verified device capacity
and measured service timing. No capacity is inferred from a nominal card model.

Prefill validation: all139 native executables build; the configured-prefill
register and editor fixtures pass shared030 with19 and171 Fast allocations
respectively, all released. Complete host invocation passes203 cases; the one
HEAD-based oracle was separately rerun successfully on8cefe39 in7.839s to cover
this latest source.
Evidence and the exact source caveat: `evidence/enhanced-editor/studio-prefill/`.

## Read-only native Mini register qualification (1 October2026)

`src/native/amigus_pcm_read.c` supplies five volatile16-bit status reads only
(flags0x00, mask0x02, format0x04, rate0x06, pendingwords0x10). It requires an
exclusive PCM reservation/access lease, no installed interrupt, and the observed
Mini hardware0/firmware0x7ea663e7. Unknown cards/firmware, unaligned/wrapping/null
addresses, lost ownership and other offsets refuse without changing the output.
No write/reset/start/IRQ or capacity guess is exposed.

AmiGUSTest0.4 `--idle-registers` uses the approved unused-AHI window around this
probe. It reserves through the production native adapter/core lifecycle, reads
status, reports PASS only for enableclear/playbackmaskclear/zero pendingwords,
ends access, explicitly confirms its own release with the pinned NULL-owner
probe, then closes the library and restores AHI. Unconfirmed release/restoration
retains Task/base/card/owner and the harness target/files. Non-idle observations
are SKIP, with no attempt to change another owner's previous hardware state.

Shared030 synthetic-memory fixture passes five reads/sixteen refusal guards and
unchanged backing bytes. Exact0.4 four missing-library modes return5; existing
guard/Exec/ownership/ABI fixtures pass. Exact cleanup and subsequent independent
absence/running68030/allDMAoff pass, window released. Evidence:
`../evidence/enhanced-editor/amigus-pcm-read-native/`. The separately coordinated realMini read-only probe now PASS: run1790892657172678000,
flags/mask/format/rate/pendingwords all0, ownrelease confirmed1/retained0, normal
AHIreload resident1/support1/delayed0/users0, unchanged installeddriverCRC. Exact
RAMcleanup/separateabsence/connectedidle030return PASS and physicalwindowreleased.
Evidence: `../evidence/enhanced-editor/amigus-pcm-read-physical/`. This qualifies
these five status reads in the observed idle state. Reset/write/readback changes,
FIFOcapacity/ordering, IRQ, playback, physicalsilence and listening remain open.
